#pragma once

#include <string>
#include <iosfwd>
#include <optional>

namespace stc
{

/// Reads the file's content specified by its path without modification.
/// Returns nullopt on error.
std::optional<std::string> read_file(const std::string &path);

}