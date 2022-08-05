#pragma once

///
/// \file
/// \brief Defines consume() for reading built-in types from documents.
///

#include <climits>
#include <cstdint>
#include <cassert>
#include <charconv>
#include <type_traits>

#if _MSC_VER >= 1924 || __GNUC__ >= 11
#define STC_HAS_FLOAT_FROM_CHARS //supports std::from_chars with floats
#else
#include <cmath>
#include <cstdlib> //uses std::stof as fallback
#include <cstring>
#endif

#include "doc_consumer.hpp"
#include "arithmetic_utilities.hpp"

namespace stc
{

inline bool consume(type_wrap<bool>, doc_input::token_kind first, doc_input &input, const doc_context &context)
{
    if(first != doc_input::token_kind::boolean && !input.hint(doc_input::token_kind::boolean))
    {
        context.error_handler(doc_error{ input.location(), doc_error::kind::type_mismatch });
        throw doc_consume_exception();
    }

    return input.boolean();
}

/// Integers and floats. Both may be specified using an exponent, but only floats may have fractional digits.
template<class T>
std::enable_if_t<std::is_arithmetic_v<T> && !std::is_same_v<T, bool> && !std::is_same_v<T, char>, T>
        consume(type_wrap<T>, doc_input::token_kind first, doc_input &input, const doc_context &context)
{
    if(first != doc_input::token_kind::number && !input.hint(doc_input::token_kind::number))
    {
        context.error_handler(doc_error{ input.location(), doc_error::kind::type_mismatch});
        throw doc_consume_exception();
    }

    ref_string n = input.raw_number();
    assert(n.size() > 0);
    const char *begin = n.data();
    const char *end = begin + n.size();

    if(std::is_unsigned_v<T> && *begin== '-')
    {
        context.error_handler(doc_error{ input.location(), doc_error::kind::value_too_small });
        throw doc_consume_exception();
    }

    T value;
    std::from_chars_result error;
    if constexpr(std::is_integral_v<T>)
    {
        error = std::from_chars(begin, end, value);
    }
    else
    {
#ifdef STC_HAS_FLOAT_FROM_CHARS
        error = std::from_chars(begin, end, value);
#else
        char tmp[64];
        std::strncpy(tmp, begin, n.size() + 1);
        tmp[63] = 0;
        char *ptr;
        if constexpr(std::is_same_v<T, float>)
        {
            value = std::strtof(tmp, &ptr);
            error.ec = value == HUGE_VALF ? std::errc::result_out_of_range : std::errc();
        }
        else if constexpr(std::is_same_v<T, double>)
        {
            value = std::strtold(tmp, &ptr);
            error.ec = value == HUGE_VAL ? std::errc::result_out_of_range : std::errc();
        }
        else if constexpr(std::is_same_v<T, long double>)
        {
            value = std::strtold(tmp, &ptr);
            error.ec = value == HUGE_VALL ? std::errc::result_out_of_range : std::errc();
        }
#endif
    }

    assert(error.ec != std::errc::invalid_argument);
    if(error.ec == std::errc::result_out_of_range)
    {
        context.error_handler(doc_error{ input.location(), doc_error::kind::value_out_of_bounds });
        throw doc_consume_exception();
    }

    if constexpr(std::is_integral_v<T>) //manually assemble number with exponent
    {
        if(error.ptr == end)
            return value;

        assert(error.ptr + 1 < end);
        if(error.ptr[0] == '.' || error.ptr[1] == '-')
        {
            context.error_handler(doc_error{ input.location(), doc_error::kind::value_out_of_bounds });
            throw doc_consume_exception();
        }

        unsigned exponent = 0;
        error = std::from_chars(error.ptr + 1, end, exponent);
        assert(error.ec != std::errc::invalid_argument);

        T powered;
        if(error.ec == std::errc::result_out_of_range || !safe_integer_power10(powered, value, exponent))
        {
            context.error_handler(doc_error{ input.location(), doc_error::kind::value_out_of_bounds });
            throw doc_consume_exception();
        }

        return powered;
    }

    return value;
}

/// Single character as a string of one code-unit.
inline char consume(type_wrap<char>, doc_input::token_kind first, doc_input &input, const doc_context &context)
{
    if(first != doc_input::token_kind::string && !input.hint(doc_input::token_kind::string))
    {
        context.error_handler(doc_error{ input.location(), doc_error::kind::type_mismatch });
        throw doc_consume_exception();
    }

    std::string_view str = input.string();
    if(str.size() != 1)
    {
        context.error_handler(doc_error{ input.location(), doc_error::kind::length_too_big });
        throw doc_consume_exception();
    }

    return str.front();
}


inline ref_string consume(type_wrap<ref_string>, doc_input::token_kind first, doc_input &input, const doc_context &context)
{
    if(first != doc_input::token_kind::string && !input.hint(doc_input::token_kind::string))
    {
        context.error_handler(doc_error{ input.location(), doc_error::kind::type_mismatch });
        throw doc_consume_exception();
    }

    return input.string();
}

}