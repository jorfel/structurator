#pragma once

#include "validation.hpp"

namespace stc
{

/// Ensures that numeric values are bounded.
/// This class must be default-constructible, so range-checking cannot be enforced in default constructor.
template<class T, size_t Min, size_t Max>
struct range_bounded_checker
{
    std::optional<doc_error::kind> operator()(const T &t) const
    {
        if(t > Max)
            return doc_error::kind::value_too_big;
        
        if(t < Min)
            return doc_error::kind::value_too_small;

        return std::nullopt;
    }
};

template<class T, T Min, T Max>
using range_bounded = validated_type<T, range_bounded_checker<T, Min, Max>>;

}