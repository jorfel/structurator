#pragma once

///
/// \file
/// \brief Defines consume() function for reading some class from the standard library from documents.
///

#include <map>
#include <array>
#include <memory>
#include <vector>
#include <optional>
#include <algorithm>

#include "doc_input.hpp"
#include "doc_consumer.hpp"

namespace stc
{

inline std::string consume(type_wrap<std::string>, doc_input::token_kind first, doc_input &input, const doc_context &context)
{
    if(first != doc_input::token_kind::string && !input.hint(doc_input::token_kind::string))
    {
        context.error_handler(doc_error{ input.location(), doc_error::kind::type_mismatch });
        throw doc_consume_exception();
    }

    return std::string(input.string());
}


template<class T>
std::optional<T> consume(type_wrap<std::optional<T>>, doc_input::token_kind first, doc_input &input, const doc_context &context)
{
    if(first == doc_input::token_kind::eof || first == doc_input::token_kind::null)
        return std::nullopt;

    return consume(type_wrap<T>(), first, input, context);
}


template<class T>
std::unique_ptr<T> consume(type_wrap<std::unique_ptr<T>>, doc_input::token_kind first, doc_input &input, const doc_context &context)
{
    return std::make_unique<T>(consume(type_wrap<T>(), first, input, context));
}


template<class T, size_t N>
std::array<T, N> consume(type_wrap<std::array<T, N>>, doc_input::token_kind first, doc_input &input, const doc_context &context)
{
    if(first != doc_input::token_kind::begin_array && !input.hint(doc_input::token_kind::begin_array))
    {
        context.error_handler(doc_error{ input.location(), doc_error::kind::type_mismatch });
        throw doc_consume_exception();
    }

    std::array<T, N> array;
    size_t count = 0;
    doc_input::token_kind token;
    while((token = input.next_token()) != doc_input::token_kind::end_array)
    {
        array[std::min(count, N - 1)] = consume(type_wrap<T>(), token, input, context);
        count++;
    }

    if(count != N)
    {
        context.error_handler(doc_error{ input.location(), count < N ? doc_error::kind::too_few_elements : doc_error::kind::too_many_elements });
        throw doc_consume_exception();
    }
    
    return array;
}


template<class T>
std::vector<T> consume(type_wrap<std::vector<T>>, doc_input::token_kind first, doc_input &input, const doc_context &context)
{
    if(first != doc_input::token_kind::begin_array && !input.hint(doc_input::token_kind::begin_array))
    {
        context.error_handler(doc_error{ input.location(), doc_error::kind::type_mismatch });
        throw doc_consume_exception();
    }

    std::vector<T> vector;

    doc_input::token_kind token;
    while((token = input.next_token()) != doc_input::token_kind::end_array)
        vector.emplace_back(consume(type_wrap<T>(), token, input, context));
    
    return vector;
}


template<class K, class V>
std::map<K, V> consume(type_wrap<std::map<K, V>>, doc_input::token_kind first, doc_input &input, const doc_context &context)
{
    static_assert(std::is_constructible_v<K, ref_string&&>);

    if(first != doc_input::token_kind::begin_mapping && !input.hint(doc_input::token_kind::begin_mapping))
    {
        context.error_handler(doc_error{ input.location(), doc_error::kind::type_mismatch });
        throw doc_consume_exception();
    }

    std::map<K, V> map;

    doc_input::token_kind token;
    while((token = input.next_token()) != doc_input::token_kind::end_mapping)
    {
        ref_string key = input.mapping_key();
        map[K(std::move(key))] = consume(type_wrap<V>(), token, input, context);
    }

    return map;
}

}