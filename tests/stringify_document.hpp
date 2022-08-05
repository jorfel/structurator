#pragma once

#include <catch2/catch.hpp>

#include <stack>
#include <string>
#include <string_view>
#include <structurator/doc_input.hpp>

inline std::string stringify_next(stc::doc_input::token_kind first, stc::doc_input &input)
{
    using tok = stc::doc_input::token_kind;
    switch(first)
    {
        case tok::eof:
            return "<eof>";

        case tok::begin_mapping:
        {
            tok next;
            std::string str = "<map>";
            while((next = input.next_token()) != tok::end_mapping)
            {
                str += '\'' + std::string(input.mapping_key()) + "'=";
                str += stringify_next(next, input);
            }

            return str + "</map>";
        }

        case tok::begin_array:
        {
            tok next;
            std::string str = "<array>";
            while((next = input.next_token()) != tok::end_array)
                str += "entry=" + stringify_next(next, input);

            return str + "</array>";
        }

        case tok::string:
            return '\'' + std::string(input.string()) + '\'';

        case tok::boolean:
            return input.boolean() ? "true" : "false";

        case tok::null:
            return "null";

        case tok::number:
        {
            return std::string(input.raw_number()) + ' ';
        }

        default:
            break;
    }

    FAIL();
    std::abort();
}

inline std::string stringify_document(stc::doc_input &input)
{
    return stringify_next(input.next_token(), input);
}