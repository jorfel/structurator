#pragma once

/// 
/// \file
/// \brief Interfaces for parsing a document.
/// 
/// Parsers may implement the doc_input interface and encapsulate the document with the generic types specified below.
/// 

#include <cstddef>
#include <cstdint>
#include <optional>

#include "ref_string.hpp"

namespace stc
{

/// Location within a source document file.
struct doc_location
{
    size_t byte; ///<Byte index from the start of the file, starting from zero.
    unsigned line; ///<Line within the file, starting at one.
};


/// Raised by a input parser when an error occurred.
struct doc_input_exception : public std::exception
{
    doc_input_exception() : exception() {}
    const char *what() const noexcept { return "doc_input_exception"; }
};


/// Reads tokens one-at-a-time from some document.
/// Tokens are enumerated depth-first.
/// begin/end-tokens must always complement each other on the same level.
struct doc_input
{
    virtual ~doc_input() = default;

    enum class token_kind
    {
        eof, ///< End of document.

        begin_mapping, ///< Begin of key->value mapping.
        end_mapping,

        begin_array, ///< Begin of consecutive values.
        end_array,

        null, ///< Generic null/nil literal.
        boolean,///< Generic boolean literal.
        number, ///< Generic number literal.
        string, ///< Generic string literal.
    };

    /// Retrieves the next token and makes it current.
    /// Throws doc_input_exception on syntax error.
    virtual token_kind next_token() = 0;

    /// Tries to convert the current token into the specified one.
    /// The parser might obey it if the current token is ambiguous and therefore was parsed as a string.
    /// Returns whether successful.
    virtual bool hint(token_kind) { return false; }

    enum class relative_loc
    {
        value, ///< Location of the current token.
        key, ///< Location of the current token's key, if any.
    };

    /// Returns a location within the parsed document as specified by the parameter.
    virtual doc_location location(relative_loc = relative_loc::value) const = 0;

    /// Returns the current key.
    /// The current token must be associated with a key.
    virtual ref_string &&mapping_key() = 0;

    /// Returns the current boolean value.
    /// The current token must be a boolean.
    virtual bool boolean() = 0;

    /// Returns the current generic number.
    /// The current token must be a number.
    /// Numbers are of form <minus><integral>.<fractional>[eE]<minus><exponent> with optional minus.
    /// The integral part is always present, the other two parts may be missing.
    virtual ref_string &&raw_number() = 0;

    /// Current string literal.
    /// The current token must be a string.
    /// ref_string ensures that no unneccesary copies of the string are made when a simple view suffices.
    virtual ref_string &&string() = 0;
};

}