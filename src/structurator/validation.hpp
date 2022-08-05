#pragma once

#include "doc_input.hpp"
#include "doc_consumer.hpp"

namespace stc
{

/// Provides simple validation of the value of an underlying type.
/// The validator simply takes the value to check and returns
/// std::optional<doc_error::kind>, which is nullopt when the value is valid.
template<class T, class Validator>
struct validated_type
{
public:
    validated_type() = default;

    validated_type(const T &t) : value(t)
    {
        assert(!Validator()(t));
    }

    validated_type(T &&t) : value(std::move(t))
    {
        assert(!Validator()(value));
    }

    validated_type &operator=(const T &t)
    {
        assert(!Validator()(t));
        value = t;
        return *this;
    }

    validated_type &operator=(T &&t)
    {
        assert(!Validator()(t));
        value = std::move(t);
        return *this;
    }

    operator T() const
    {
        assert(!Validator()(value));
        return value;
    }

private:
    T value;
};


template<class T, class Validator>
validated_type<T, Validator> consume(type_wrap<validated_type<T, Validator>>, doc_input::token_kind first, doc_input &input, const doc_context &context)
{
    T t = consume(type_wrap<T>(), first, input, context);
    std::optional<doc_error::kind> err = Validator()(t);
    if(err)
    {
        context.error_handler(doc_error{ input.location(), *err });
        throw doc_consume_exception();
    }

    return std::move(t);
}

}