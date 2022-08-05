#pragma once

#include <cstddef>
#include <climits>
#include <string_view>

#include "doc_input.hpp"

/// \file
/// \brief Defines some useful generic functions for parsing.

namespace stc
{

/// Skips to the first non-whitespace and updates the current line
template<class LineT>
inline void skip_whitespaces(std::string_view &source, LineT &line)
{
    while(!source.empty())
    {
        char ch = source.front();
        if(ch != ' ' && ch != '\t' && ch != '\f' && ch != '\r' && ch != '\n')
            return;

        if(ch == '\n')
            line++;

        source.remove_prefix(1);
    }
}

/// Computes the number represented by exactly as many hexadecimal digits as needed for T.
/// Returns whether the characters were actual hexadecimal digits.
template<class T>
bool number_from_hex(const char *ptr, T &value)
{
    value = 0;
    for(size_t i = 0; i < sizeof(T) * CHAR_BIT / 4; ++i)
    {
        char ch = *ptr++;
        unsigned digit;
        if(ch >= '0' && ch <= '9')
            digit = ch - '0';
        else if(ch >= 'a' && ch <= 'f')
            digit = ch - 'a' + 10;
        else if(ch >= 'A' && ch <= 'F')
            digit = ch - 'A' + 10;
        else
            return false;

        value = value * 16 + digit;
    }

    return true;
}


enum class number_validation_result
{
    success,
    eof,
    invalid_char,
};

/// Expects at least one decimal digits, advances the view and stops on end or non-digit character.
number_validation_result expect_digits(std::string_view &source);

/// Expects a number of form <sign><integer>.<fractional>E<sign><exponent> or with less parts.
number_validation_result expect_number(std::string_view &source);

}