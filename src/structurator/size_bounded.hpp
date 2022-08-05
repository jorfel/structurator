#pragma once

#include <cassert>
#include <climits>
#include <type_traits>

#include "doc_consumer.hpp"

namespace stc
{

/// Ensures that the length of some container is bounded.
/// This class must be default-constructible, so range-checking cannot be enforced in default constructor.
template<class T, size_t MinSize, size_t MaxSize = SIZE_MAX>
class size_bounded
{
public:
    size_bounded() = default;

    size_bounded(T t)
    {
        assert(t.size() >= MinSize);
        assert(t.size() <= MaxSize);
        *this = std::move(t);
    }

    size_bounded &operator=(T t)
    {
        container = std::move(t);
        return *this;
    }

    operator const T &() const
    {
        assert(container.size() >= MinSize);
        assert(container.size() <= MaxSize);
        return container;
    }

    auto begin() { return container.begin(); }
    auto end() { return container.end(); }

    auto begin() const { return container.begin(); }
    auto end() const { return container.end(); }

private:
    T container;
};


template<class T, size_t MinSize, size_t MaxSize>
size_bounded<T, MinSize, MaxSize> consume(type_wrap<size_bounded<T, MinSize, MaxSize>>, doc_input::token_kind first, doc_input &input, const doc_context &context)
{
    T t = consume(type_wrap<T>(), first, input, context);
    if(t.size() < MinSize || t.size() > MaxSize)
    {
        context.error_handler(doc_error{ input.location(), t.size() < MinSize ? doc_error::kind::length_too_small : doc_error::kind::length_too_big });
        throw doc_consume_exception();
    }

    return size_bounded<T, MinSize, MaxSize>(std::move(t));
}


}