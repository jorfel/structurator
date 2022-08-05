#include "parse_utilities.hpp"

namespace stc
{

number_validation_result expect_digits(std::string_view &source)
{
    size_t count = 0;
    for(; !source.empty() && source.front() >= '0' && source.front() <= '9'; ++count)
        source.remove_prefix(1);

    if(count == 0)
    {
        if(source.empty())
            return number_validation_result::eof;
        else
            return number_validation_result::invalid_char;
    }

    return number_validation_result::success;
}

number_validation_result expect_number(std::string_view &source)
{
    if(source.front() == '-')
        source.remove_prefix(1);

    if(auto res = expect_digits(source); res != number_validation_result::success)
        return res;

    if(!source.empty() && source.front() == '.')
    {
        source.remove_prefix(1);
        if(auto res = expect_digits(source); res != number_validation_result::success)
            return res;
    }

    if(!source.empty() && (source.front() == 'e' || source.front() == 'E'))
    {
        source.remove_prefix(1);
        if(source.empty())
            return number_validation_result::eof;

        if(source.front() == '-')
            source.remove_prefix(1);

        if(auto res = expect_digits(source); res != number_validation_result::success)
            return res;
    }

    return number_validation_result::success;
}

}