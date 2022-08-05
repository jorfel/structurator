#include <string>
#include <cstddef>
#include <string_view>

#include "utf8.hpp"

namespace stc
{

/// Default function for stringifying enumeration values.
template<class T>
inline std::string enum_string(T e)
{
    return std::to_string(unsigned(e));
}


/// Returns a simple message for some error object.
/// The stringification of error codes are delegated to their appropriate enum_string.
template<class T>
std::string error_string(std::string_view document, const T &error)
{
    size_t column = utf8_line_column(std::string_view(document.data(), error.location.byte));
    return "Line " + std::to_string(error.location.line) + ": " + std::string(enum_string(error.what)) + '\n' +
        std::string(stc::utf8_complete_line(document, error.location.byte)) + '\n' +
        std::string(column, '-') + "^\n";
}


}