#include "json_input.hpp"

#include <stack>
#include <vector>
#include <string>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <variant>
#include <charconv>
#include <algorithm>
#include <exception>

#include "utf8.hpp"
#include "ref_string.hpp"
#include "parse_utilities.hpp"


namespace stc::json
{

static constexpr size_t max_errors = 16;


/// Whether the specified character is invalid in JSON strings.
static bool is_control_char(char c)
{
    return c >= 0 && c < 32;
}

/// Whether the specified character can follow a backslash as an escape sequence in JSON strings.
static bool is_escapable_char(char c)
{
    return std::strchr("\"\\/bfnrtu", c) != nullptr;
}

/// Replaces all escape sequences and returns a new string.
/// Unicode sequences are replaced by UTF-8 code-units.
/// Leaves unknown escape sequences untouched.
static std::string unescape_string(std::string_view &string)
{
    const char *specials = "\"\\/bfnrt";
    const char *replacements = "\"\\/\b\f\n\r\t";

    std::string result;
    result.reserve(string.size() * 6 / 5); //heuristically reserve with one fifth more space
    while(!string.empty())
    {
        if(string.size() < 2 || string.front() != '\\') //no escape sequence
        {
            result += string.front();
            string.remove_prefix(1);
            continue;
        }

        if(const char *s = std::strchr(specials, string[1]); s != nullptr) //ordinary escape sequence \x
        {
            result += replacements[s - specials];
            string.remove_prefix(2);
            continue;
        }

        char16_t codepoint1;
        if(string.size() >= 6 && (string[1] == 'u' || string[1] == 'U') && number_from_hex(&string[2], codepoint1)) //unicode escape sequence \uxxxx
        {
            char16_t codepoint2;
            if(string.size() >= 12 && string[6] == '\\' && (string[7] == 'u' || string[7] == 'U') && //surrogate pair \uxxxx\uxxxx
                is_surrogate1(codepoint1) &&
                number_from_hex(&string[8], codepoint2) &&
                is_surrogate2(codepoint2))
            {
                encode_utf8(result, from_surrogate_pair(codepoint1, codepoint2));
                string.remove_prefix(6);
            }
            else
            {
                encode_utf8(result, codepoint1);
            }

            string.remove_prefix(6);
            continue;
        }

        if(!string.empty()) //unknown escape sequence, leave untouched
        {
            result += string.front();
            string.remove_prefix(1);
        }
    }

    return result;
}


/// Parses the given JSON string, starting after the initial quote and stopping after the ending quote.
static std::variant<parse_error::kind, ref_string> parse_string_literal(std::string_view &source)
{
    const char *begin = source.data();
    bool needs_escape = false;
    while(!source.empty())
    {
        while(!source.empty() && source.front() != '\\' && source.front() != '"' && !is_control_char(source.front())) //skip ordinary characters
            source.remove_prefix(1);

        if(source.empty())
            break;

        char ch = source.front();
        if(ch == '\n')
            return parse_error::kind::string_invalid_newline;

        if(is_control_char(ch))
            return parse_error::kind::string_invalid_char;

        if(ch == '\\')
            needs_escape = true;

        else if(ch == '"')
        {
            std::string_view all(begin, source.data() - begin);
            source.remove_prefix(1);
            if(needs_escape)
                return ref_string::make_copy(unescape_string(all));
            else
                return ref_string(all);
        }

        source.remove_prefix(1);
    }

    return parse_error::kind::eof_unexpected;
}


/// Skips to the next ] or } on the same level of parentheses/string quotes.
static void skip_container(std::string_view &source, std::uint32_t &line)
{
    size_t level = 0;
    bool inside_string = false;
    char last_char = 0;
    while(!source.empty())
    {
        char c = source.front();
        if(level == 0 && (c == ']' || c == '}'))
        {
            source.remove_prefix(1);
            break;
        }

        switch(c)
        {
            case '[':
            case '{': level++; break;

            case ']':
            case '}':
                if(!inside_string)
                    level--;
                break;

            case '"':
                if(last_char != '\\')
                    inside_string = !inside_string;
                break;


            case '\n':
                line++;
                if(inside_string)
                    inside_string = false; //always terminate string literals at end of line
                break;
        }

        last_char = source.front();
        source.remove_prefix(1);
    }
}


/// Parses JSON document using recursive descent.
/// Switches between different parse_* methods on each call to next_token() and uses a
/// stack to store information when entering arrays or objects.
/// In case of an error, the current object or array is skipped, parsing is continued
/// and more errors are detected.
struct parser : public doc_input
{
    const char *source_begin;
    std::string_view source;
    parse_error_handler error_handler;

    parser(std::string_view s, parse_error_handler e) : source(s), error_handler(std::move(e))
    {
        source_begin = source.data();
        call_stack.reserve(16);

        skip_whitespaces(source, line);
        next_call = &parser::parse_begin;
    }

    std::uint32_t line = 1;

    using parse_func_t = token_kind(parser::*)();
    parse_func_t next_call = &parser::parse_any;

    struct stack_entry
    {
        const char *from;
        parse_func_t next_call;
        std::uint32_t line;
    };

    std::vector<stack_entry> call_stack;

    const char *property_begin = nullptr;
    const char *value_begin = nullptr;

    ref_string current_property;
    ref_string current_string;
    ref_string current_number;
    bool current_bool = false;

    size_t error_count = 0;


    doc_location location_at(const char *relative_to) const
    {
        size_t byte = relative_to - source_begin;
        size_t excess_lines = std::count(relative_to, source.data(), '\n');
        return doc_location{ byte, unsigned(line - excess_lines) };
    }

    void push_stack()
    {
        call_stack.emplace_back(stack_entry{ source.data(), next_call, line });
    }

    void pop_stack()
    {
        assert(!call_stack.empty());
        next_call = call_stack.back().next_call;
        call_stack.pop_back();
    }

    void raise_error(parse_error::kind what)
    {
        error_handler({ what, location_at(source.data()) });

        if(error_count < max_errors && //limit potential recursion when detecting more errors
            call_stack.size() >= 2) //only recover when within a second object/array, as detecting more errors outside root values is not sensible
        {
            error_count++;

            const auto &rec = call_stack.back();
            source = std::string_view(rec.from, source.data() + source.size() - rec.from);
            line = rec.line;
            next_call = rec.next_call;
            call_stack.pop_back();

            skip_container(source, line); //skip to end of errorneous container
            while(next_token() != token_kind::eof) //detect other errors
                ;
        }

        next_call = &parser::parse_eof;
        throw doc_input_exception();
    }

    void expect_input()
    {
        if(source.empty())
            raise_error(parse_error::kind::eof_unexpected);
    }

    token_kind parse_begin()
    {
        next_call = &parser::parse_eof;
        return source.empty() ? parse_eof() : parse_any();
    }

    token_kind parse_eof()
    {
        return token_kind::eof;
    }

    token_kind parse_any()
    {
        skip_whitespaces(source, line);
        expect_input();

        value_begin = source.data();

        char ch = source.front();
        if(ch == '{')
            return parse_object();

        if(ch == '[')
            return parse_array();

        if(ch == '"')
            return parse_string();

        if(source.substr(0, 4) == "true")
            return parse_bool<true>();

        if(source.substr(0, 5) == "false")
            return parse_bool<false>();

        if(source.substr(0, 4) == "null")
            return parse_null();

        return parse_number();
    }

    token_kind parse_object()
    {
        source.remove_prefix(1);
        skip_whitespaces(source, line);
        expect_input();

        push_stack();
        next_call = &parser::parse_property<true>;
        return token_kind::begin_mapping;
    }

    template<bool AllowEnd>
    token_kind parse_property()
    {
        char ch = source.front();
        if(AllowEnd && ch == '}')
        {
            value_begin = source.data();
            source.remove_prefix(1);
            pop_stack();
            return token_kind::end_mapping;
        }

        if(ch != '"')
            raise_error(parse_error::kind::expected_key);

        source.remove_prefix(1);
        property_begin = source.data();

        auto string_result = parse_string_literal(source);
        if(auto *error = std::get_if<parse_error::kind>(&string_result); error != nullptr)
            raise_error(*error);

        current_property = std::move(*std::get_if<ref_string>(&string_result));

        skip_whitespaces(source, line);
        expect_input();

        if(source.front() != ':')
            raise_error(parse_error::kind::expected_colon);

        source.remove_prefix(1);
        next_call = &parser::parse_next_property;
        return parse_any();
    }

    token_kind parse_next_property()
    {
        skip_whitespaces(source, line);
        expect_input();

        char ch = source.front();
        if(ch != ',' && ch != '}')
            raise_error(parse_error::kind::expected_separator);

        if(ch == ',')
        {
            source.remove_prefix(1);
            skip_whitespaces(source, line);
            expect_input();
            return parse_property<false>();
        }
        
        return parse_property<true>();

    }

    token_kind parse_array()
    {
        source.remove_prefix(1);
        skip_whitespaces(source, line);
        expect_input();

        push_stack();
        next_call = &parser::parse_array_entry<true>;
        return token_kind::begin_array;
    }

    template<bool AllowEnd>
    token_kind parse_array_entry()
    {
        if(AllowEnd && source.front() == ']')
        {
            value_begin = source.data();
            source.remove_prefix(1);
            pop_stack();
            return token_kind::end_array;
        }
        
        next_call = &parser::parse_next_array_entry;
        return parse_any();
    }

    token_kind parse_next_array_entry()
    {
        skip_whitespaces(source, line);
        expect_input();

        char ch = source.front();
        if(ch != ',' && ch != ']')
            raise_error(parse_error::kind::expected_separator);

        if(ch == ',')
        {
            source.remove_prefix(1);
            skip_whitespaces(source, line);
            expect_input();
            return parse_array_entry<false>();
        }

        return parse_array_entry<true>();
    }

    token_kind parse_string()
    {
        source.remove_prefix(1);

        auto string_result = parse_string_literal(source);
        if(auto *error = std::get_if<parse_error::kind>(&string_result); error != nullptr)
            raise_error(*error);

        current_string = std::move(*std::get_if<ref_string>(&string_result));
        return token_kind::string;
    }

    template<bool Value>
    token_kind parse_bool()
    {
        source.remove_prefix(Value ? 4 : 5);
        current_bool = Value;
        return token_kind::boolean;
    }

    token_kind parse_null()
    {
        source.remove_prefix(4);
        return token_kind::null;
    }

    token_kind parse_number()
    {
        const char *begin = source.data();
        auto res = expect_number(source);
        if(res == number_validation_result::eof)
            raise_error(parse_error::kind::eof_unexpected);

        else if(res == number_validation_result::invalid_char)
            raise_error(parse_error::kind::string_invalid_char);

        current_number = std::string_view(begin, source.data() - begin);
        return token_kind::number;
    }
    
    //implementation of doc_input
    doc_location location(relative_loc rel) const override
    {
        const char *relative_to = rel == relative_loc::value ? value_begin : property_begin;
        assert(relative_to != nullptr);
        return location_at(relative_to);
    }

    token_kind next_token() override
    {
        assert(next_call != nullptr);
        return (this->*next_call)();
    }

    ref_string &&mapping_key() override
    {
        return std::move(current_property);
    }

    bool boolean() override
    {
        return current_bool;
    }

    ref_string &&raw_number() override
    {
        return std::move(current_number);
    }

    ref_string &&string() override
    {
        return std::move(current_string);
    }
};


std::unique_ptr<doc_input> input(std::string_view source, parse_error_handler handler)
{
    return std::make_unique<parser>(source, std::move(handler));
}


}