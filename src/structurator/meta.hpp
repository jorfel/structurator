#pragma once

namespace stc
{

/// Wraps another type to argument-dependent-lookup a function for that type.
template<class T>
struct type_wrap
{
    using type = T;
};


struct not_present_t
{
    constexpr bool operator==(not_present_t) const { return true; }
    constexpr bool operator!=(not_present_t) const { return false; }

};

/// Value that indicates some function did not found what was searched for.
/// Is not equal to any other object but only to itself.
static constexpr not_present_t not_present;

template<class T>
constexpr bool operator==(not_present_t, T) { return false; }

template<class T>
constexpr bool operator!=(T, not_present_t) { return true; }

}