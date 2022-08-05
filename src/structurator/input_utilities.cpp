#include <climits>
#include <fstream>
#include <filesystem>

#include "input_utilities.hpp"


namespace stc
{


std::optional<std::string> read_file(const std::string &path)
{
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if(!in)
        return std::nullopt;
     
    if(!in.seekg(0, std::ios::end))
        return std::nullopt;

    auto length = in.tellg();
    if(length == std::ifstream::pos_type(-1) || length > SIZE_MAX)
        return std::nullopt;

    std::string contents;
    contents.resize(size_t(length));
    in.seekg(0, std::ios::beg);
    if(!in.read(contents.data(), contents.size()))
        return std::nullopt;

    return contents;
}


}