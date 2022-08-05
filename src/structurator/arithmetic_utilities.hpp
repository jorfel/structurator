#pragma once

/// 
/// \file
/// \brief Defines some overflow-safe function for performing arithmetic.
/// 

#ifndef __has_builtin
    #define __has_builtin(x) 0
#endif

#if !__has_builtin(__builtin_mul_overflow)

    #if __has_include(<intprops.h>)
        #include <intprops.h>
    #elif __has_include(<x86intrin.h>)
        #include <x86intrin.h>
    #elif __has_include(<intrin.h>)
        #include <intrin.h>
        #pragma intrinsic(__mulh)
        #pragma intrinsic(__umulh)
    #endif
#endif

namespace stc
{

/// Multiplies two integers and returns true when successful or false if it would overflow.
template<class T>
bool safe_integer_mul(T &result, T a, T b)
{
    static constexpr unsigned bits = sizeof(T) * CHAR_BIT;

#if __has_builtin(__builtin_mul_overflow)
    return !__builtin_mul_overflow(a, b, &result);

#elif __has_include(<intprops.h>)
    if(!INT_MULTIPLY_OVERFLOW(a, b))
    {
        result = a * b;
        return true;
    }
    return false;

#elif (__has_include(<x86intrin.h>) || __has_include(<intrin.h>))
    if constexpr(bits == 64)
    {
        if constexpr(std::is_signed_v<T>)
        {
            if(__mulh(a, b) == 0)
            {
                result = a * b;
                return true;
            }
        }
        else
        {
            if(__umulh(a, b) == 0)
            {
                result = a * b;
                return true;
            }
        }
        return false;
    }
#endif

#if defined(INT64_C)
    if constexpr(bits <= 32)
    {
        auto r = std::int64_t(a) * std::int64_t(b);
        if(r >= std::numeric_limits<T>::min() && r <= std::numeric_limits<T>::max())
        {
            result = a * b;
            return true;
        }
        return false;
    }
#endif

    return true;
}

/// Raises a value \b to the power of \e. Returns true when successful or false if it would overflow.
template<class T>
bool safe_integer_power(T &result, T b, unsigned e)
{
    result = 1;
    while(e != 0)
    {
        if(e & 1)
        {
            if(!safe_integer_mul(result, b, result))
                return false;
        }

        if(e > 1 && !safe_integer_mul(b, b, b))
            return false;
        
        e /= 2;
    }

    return true;
}

/// Raises a value \b to the power of 10**\e. Returns true when successful or false if it would overflow.
template<class T>
bool safe_integer_power10(T &result, T b, unsigned e)
{
    T mul;
    if(!safe_integer_power(mul, T(10), e))
        return false;

    return safe_integer_mul(result, b, mul);
}

}