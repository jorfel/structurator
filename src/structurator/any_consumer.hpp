#pragma once

#include <any>
#include <map>
#include <string>
#include <vector>
#include <cstdlib>

#include "doc_input.hpp"
#include "doc_consumer.hpp"

#include "native_consumers.hpp"
#include "stdlib_consumers.hpp"


namespace stc
{

/// Defines consume() for reading std::any from documents.
inline std::any consume(type_wrap<std::any>, doc_input::token_kind first, doc_input &input, const doc_context &context)
{
    using tok = doc_input::token_kind;
    switch(first)
    {
        case tok::null: return std::any();
        case tok::boolean: return consume(type_wrap<bool>(), first, input, context);
        case tok::number: return consume(type_wrap<double>(), first, input, context);
        case tok::string: return consume(type_wrap<std::string>(), first, input, context);
        case tok::begin_mapping: return consume(type_wrap<std::map<std::string, std::any>>(), first, input, context);
        case tok::begin_array: return consume(type_wrap<std::vector<std::any>>(), first, input, context);
        default:
            std::abort();
    }
}

}