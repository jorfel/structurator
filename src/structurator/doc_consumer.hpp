#pragma once

///
/// \file
/// \brief Defines an interface to use when reading documents from some input source.
/// 
/// Documents are read using consume() functions for various types.
/// Errors are handled by given it to an error handler and then raising a doc_consume_exception.
///

#include <exception>
#include <functional>
#include <type_traits>

#include "meta.hpp"
#include "doc_input.hpp"

namespace stc
{

/// Generic error descriptor.
struct doc_error
{
    doc_location location;

    enum class kind
    {
        type_mismatch,
        type_unspecified,
        value_invalid,
        value_out_of_bounds,
        value_too_small,
        value_too_big,
        value_unknown,
        length_too_small,
        length_too_big,
        too_few_elements,
        too_many_elements,
        key_unknown,
        key_duplicate,
        key_missing,
        index_out_of_bounds,
    } what;
};

#ifdef STC_DEFINE_MESSAGES
inline std::string_view enum_string(doc_error::kind what)
{
    static const char *msgs[] = {
        "Value is of wrong type.",
        "The type of this value was not specified.",
        "Value does not meet required criteria.",
        "Value is not within the allowed range.",
        "Value is too small.",
        "Value is too large.",
        "Value is not recognized here.",
        "Value is too short.",
        "Value is too long.",
        "Too few elements.",
        "Too many elements.",
        "Key is unrecognized here.",
        "Key is duplicated.",
        "Not all required keys are specified.",
        "Value is not a valid index.",
    };

    return msgs[unsigned(what)];
}
#endif


using doc_error_handler = std::function<void(const doc_error&)>;


/// Object which is passed to every consume()-function.
/// This class may be inherited and equipped for custom consume() functions.
struct doc_context
{
    doc_error_handler error_handler;
};

/// Raised by a consume() function when an error occurred.
struct doc_consume_exception : public std::exception
{
    doc_consume_exception() : exception() {}
    const char *what() const noexcept { return "doc_consume_exception"; }
};


/// Default consume() function for unknown types.
template<class T>
T consume(T, doc_input::token_kind, doc_input&, const doc_context&)
{
    if constexpr(std::is_class_v<T>)
        static_assert(std::is_same_v<T, void>, "This type does not have an appropriate consume() function, maybe due to a missing stc_declare_class().");
    else
        static_assert(std::is_same_v<T, void>, "This type does not have an appropriate consume() function.");
}


}