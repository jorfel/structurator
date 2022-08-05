#pragma once

#include <memory>
#include <functional>
#include <string_view>

#include "doc_input.hpp"

namespace stc::json
{

/// Information about errors that might occur during parsing.
struct parse_error
{
    enum class kind
    {
        eof_unexpected,
        expected_key,
        expected_colon,
        expected_separator,
        string_invalid_newline,
        string_invalid_char,
        string_invalid_escape,
    } what; ///< Type of error.

    doc_location location;
};

#ifdef STC_DEFINE_MESSAGES
inline std::string_view enum_string(parse_error::kind what)
{
    static const char *msgs[] = {
        "Unexpected end.",
        "Expected '\"' here to denote a key.",
        "Expected ':' here to denote the value of the key.",
        "Expected ',' or ']' here to denote the next entry or the end of the array.",
        "Invalid new-line in string literal.",
        "Invalid character in string literal.",
        "Invalid escape sequence in string literal."
    };

    return msgs[unsigned(what)];
}
#endif

using parse_error_handler = std::function<void(const parse_error&)>;

/// Parses the given source. On error, calls the specified handler and tries to uncover more errors.
std::unique_ptr<doc_input> input(std::string_view source, parse_error_handler handler);

}