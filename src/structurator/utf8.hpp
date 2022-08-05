#pragma once

#include <string>
#include <cstddef>
#include <string_view>

/// \file
/// \brief Provides useful functions for reading and writing UTF-8-encoded strings.

namespace stc
{

/// Decodes the first code-point within the UTF-8 string and advances the string-view.
/// The string must be at least one code-unit long.
char32_t decode_utf8(std::string_view &str);

/// Encodes the given code-point as UTF-8 and appends it to the given string.
void encode_utf8(std::string &str, char32_t cp);

/// Returns the number of glyphs since the last new-line or since the begin of the
/// given UTF-8 string, starting at the end of the string.
size_t utf8_line_column(std::string_view str);

/// Returns the complete line containing the code-point at the specified byte index.
/// Excludes the preceeding and terminating new-line if present and excludes the
/// code-point at \p index itself if it is a new-line.
std::string_view utf8_complete_line(std::string_view str, size_t idx);

/// Returns whether this is the first code-unit of a UTF-16 surrogate pair.
inline bool is_surrogate1(char16_t cp)
{
	return cp >> 10 == 0b110110;
}

/// Returns whether this is the second code-unit of a UTF-16 surrogate pair.
inline bool is_surrogate2(char16_t cp)
{
	return cp >> 10 == 0b110111;
}

/// Converts the given UTF-16 surrogate pair to a single code-point.
inline char32_t from_surrogate_pair(char16_t cp1, char16_t cp2)
{
	return ((cp1 & 0b1111111111) << 10 | (cp2 & 0b1111111111)) + 0x10000;
}

}