#pragma once

#include <optional>

#include "doc_input.hpp"
#include "doc_consumer.hpp"
#include "any_consumer.hpp"
#include "object_consumer.hpp"
#include "stdlib_consumers.hpp"
#include "native_consumers.hpp"

namespace stc
{

/// Reads an object of some type from the specified input.
/// A custom context object may be specified.
/// If the input is empty or an error occurred, std::nullopt is returned.
/// User-defined classes need to be declared first using stc_declare_class().
template<class T, class Context>
std::optional<T> from_input_with_context(doc_input &input, Context &context)
{
    static_assert(std::is_base_of_v<doc_context, Context>, "The specified context class must be derived from doc_context.");
    try
    {
        auto first = input.next_token();
        if(first != doc_input::token_kind::eof)
            return consume(type_wrap<T>(), first, input, context);
    }
    catch(const doc_input_exception&) //error handlers already called, just wrap and return nullopt
    {
    }
    catch(const doc_consume_exception&)
    {
    }

    return std::nullopt;
}

/// Simple wrapper when not specifying a custom context.
template<class T>
std::optional<T> from_input(doc_input &input, doc_error_handler handler)
{
    doc_context context{ std::move(handler) };
    return from_input_with_context<T>(input, context);
}

}