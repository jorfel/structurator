#pragma once

/// \file
/// \brief Defines stc_declare_class() and some flags and attributes to describe how to map user-defined classes.
/// 
/// Attributes are classes with a defined "tag" by which they are retrieved. Flags and attributes can be joined using |.
///

#include <array>
#include <tuple>
#include <cstddef>
#include <type_traits>
#include <string_view>

#include "meta.hpp"

namespace stc
{

/// Flags that modify the processing of class members.
enum class member_flag
{
    none = 0,
    maybe_default = 1, ///<The specified member may be absent from the document or null and retain its default value.
    additional_keys = 2, ///<The specified member is an associative container in which unknown keys and their values are inserted.
    first_of_multiple = 4, ///<Use first occurrence when duplicate keys are present.
    last_of_multiple = 8, ///<Use last occurrence when duplicate keys are present.
    multiple = 16 ///<Collect all occurences in a sequence container.
};


/// Contains flags and attributes.
template<class... AttribT>
struct member_options
{
    unsigned flags = 0;
    std::tuple<AttribT...> attributes;
};

inline constexpr auto operator|(member_flag lhs, member_flag rhs)
{
    return member_options<>{ unsigned(lhs) | unsigned(rhs) };
}

template<class... AttribT>
constexpr auto operator|(member_options<AttribT...> lhs, member_flag rhs)
{
    return lhs.flags |= unsigned(rhs), lhs;
}

template<class... AttribT>
constexpr auto operator|(member_flag lhs, member_options<AttribT...> rhs)
{
    return rhs.flags |= unsigned(lhs), rhs;
}

template<class... AttribsT, class NewAttribT>
constexpr auto operator|(member_flag lhs, NewAttribT rhs)
{
    return member_options<AttribsT..., NewAttribT>{ unsigned(lhs), std::tuple(rhs) };
}

template<class... AttribT, class NewAttribT>
constexpr auto operator|(NewAttribT lhs, member_flag rhs)
{
    return member_options<AttribT..., NewAttribT>{ unsigned(rhs), std::tuple(lhs) };
}

template<class... AttribT, class NewAttribT>
constexpr auto operator|(member_options<AttribT...> lhs, NewAttribT rhs)
{
    return member_options<AttribT..., NewAttribT>{ lhs.flags, std::tuple_cat(lhs.attributes, std::tuple(rhs)) };
}

template<class... AttribT, class NewAttribT>
constexpr auto operator|(NewAttribT lhs, member_options<AttribT...> rhs)
{
    return member_options<AttribT..., NewAttribT>{ rhs.flags, std::tuple_cat(rhs.attributes, std::tuple(lhs)) };
}

template<class AttribT1, class AttribT2>
inline constexpr auto operator|(AttribT1 lhs, AttribT2 rhs)
{
    return member_options<AttribT1, AttribT2>{ 0, std::tuple(lhs, rhs) };
}


/// Given member options, returns an attribute with \p Tag as its tag.
/// If no such attribute exists, returns not_present.
template<class Tag, size_t Idx = 0, class... AttribT>
constexpr const auto &get_member_attr(const member_options<AttribT...> &mopts)
{
    if constexpr(Idx < sizeof...(AttribT))
    {
       const auto &p = std::get<Idx>(mopts.attributes);
        if constexpr(std::is_same_v<typename std::remove_const_t<std::remove_reference_t<decltype(p)>>::tag, Tag>)
            return p;
        else
            return get_member_attr<Tag, Idx + 1, AttribT...>(mopts);
    }
    else
    {
        return not_present;
    }
}


struct member_short_tag {};

/// Attribute which specifies a different name for a certain member.
struct member_short
{
    using tag = member_short_tag;

    std::string_view short_name;
    constexpr member_short(std::string_view name) : short_name(name) {}
};


struct member_alias_tag {};

/// Attribute which specifies an alternative name for a certain member.
struct member_alias
{
    using tag = member_alias_tag;

    std::string_view alias_name;
    constexpr member_alias(std::string_view name) : alias_name(name) {}
};


/// Alternative type that is indicated by a discriminative value.
template<class AltT, class DiscrT>
struct alt_type
{
    DiscrT discriminative;
    constexpr alt_type(DiscrT d) : discriminative(std::move(d)) {}

    using alter_type = AltT;
};


/// Specifies how to consume a type.
enum class alt_mode
{
    nest, ///<Descend into a certain key-value mapping and use these keys to fill members.
    no_nesting ///<Use the remaining keys of the current key-value mapping to fiill members.
};

struct member_alts_tag {};

/// Maps discriminative values a specified key can assume to different types using alt_type.
/// The actual type to consume is dynamically chosen using a certain key in a key-value mapping.
template<class DiscrT, class... AltTypes>
struct member_alts
{
    using tag = member_alts_tag;

    std::string_view key; ///<Key of a key-value mapping to check for determining the concrete type.
    std::tuple<alt_type<AltTypes, DiscrT>...> alternatives; ///<Mappings of values to alternative types.
    alt_mode mode; ///<How to consume the choosen type.

    constexpr member_alts(std::string_view key, alt_mode mode, alt_type<AltTypes, DiscrT> ...pairs)
        : key(key), mode(mode), alternatives(std::move(pairs)...) {}

    static constexpr size_t alts_count = sizeof...(AltTypes);
    using discr_type = DiscrT;
};

/// Defines an type alternative for use with member_alts.
/// Discriminative values of type const char* are converted to std::string_view.
template<class AltT, class DiscrT>
constexpr auto alt(DiscrT d)
{
    if constexpr(std::is_same_v<DiscrT, const char*> || std::is_same_v<DiscrT, char*>)
        return alt_type<AltT, std::string_view>(std::string_view(d));
    else
        return alt_type<AltT, DiscrT>(std::move(d));
}


/// Contains information about a member from a certain class.
template<class Class, class MemberType, class MemberOptionsT>
struct member_info
{
    MemberType Class::*member_ptr;
    std::string_view name;
    MemberOptionsT options;

    using member_type = MemberType;
};


/// Contains informatin about a certain class.
template<class Type, class... MembersInfo>
struct class_info
{
    static constexpr size_t members_count = sizeof...(MembersInfo);
    std::tuple<MembersInfo...> members;
};


namespace detail
{

template<class Class, class MemberType>
static constexpr auto make_member_info(MemberType Class:: *mptr, std::string_view name, member_flag flag)
{
    return member_info<Class, MemberType, member_options<>>{ mptr, name, member_options<>{ unsigned(flag) } };
}

template<class Class, class MemberType, class... AttribT>
static constexpr auto make_member_info(MemberType Class:: *mptr, std::string_view name, member_options<AttribT...> options)
{
    return member_info<Class, MemberType, member_options<AttribT...>>{ mptr, name, options };
}

template<class Class, class MemberType, class AttribT>
static constexpr auto make_member_info(MemberType Class::*mptr, std::string_view name, AttribT attr)
{
    return member_info<Class, MemberType, member_options<AttribT>>{ mptr, name, member_options<AttribT>{ 0, attr } };
}



template<class Type, class... MembersInfo>
static constexpr auto make_class_info(MembersInfo ...members_info)
{
    return class_info<Type, MembersInfo...>{ std::tuple(members_info...) };
}


template<class T>
static constexpr decltype(T::stc_class_info(type_wrap<T>()), true) has_class_info_method(type_wrap<T>)
{
    return true;
}

static constexpr bool has_class_info_method(...)
{
    return false;
}

template<class T>
static constexpr decltype(stc_class_info(type_wrap<T>()), true) has_class_info_adlfunc(type_wrap<T>)
{
    return true;
}

static constexpr bool has_class_info_adlfunc(...)
{
    return false;
}

} //end of detail


/// Returns class information for the specified class or not_present when stc_declare_class() was not used.
/// Class informatin is gathered using either a method or an ADL-found function.
template<class T>
static constexpr auto get_class_info()
{
    if constexpr(detail::has_class_info_adlfunc(type_wrap<T>()))
        return stc_class_info(type_wrap<T>());

    else if constexpr(detail::has_class_info_method(type_wrap<T>()))
        return T::stc_class_info(type_wrap<T>());

    else
        return not_present;
}


#define STC_EXPAND(x) x

//STC_PAREN flattens x when it is surrounded by (), prepends "prefix" and encloses with ()
//see https://www.mikeash.com/pyblog/friday-qa-2015-03-20-preprocessor-abuse-and-optional-parentheses.html
#define STC_NOTHING_STC_EXTRACT
#define STC_EXTRACT(...) STC_EXTRACT __VA_ARGS__
#define STC_PASTE(macro, ...) macro ## __VA_ARGS__
#define STC_EVAL_PASTE(macro, ...) STC_PASTE(macro, __VA_ARGS__)
#define STC_PAREN(prefix, x) (prefix, STC_EVAL_PASTE(STC_NOTHING_, STC_EXTRACT x))

//STC_M1 inserts a make_member_info call without additional options, STC_M2 does with user-specified options.
#define STC_M1(type, member_name) ::stc::detail::make_member_info(&type::member_name, #member_name, ::stc::member_options{})
#define STC_M2(type, member_name, opts) ::stc::detail::make_member_info(&type::member_name, #member_name, (opts))


//STC_EXPAND solves an issue with MSVC and must be used with __VA_ARGS__
//see https://stackoverflow.com/questions/5134523/msvc-doesnt-expand-va-args-correctly
#ifdef _MSC_VER


//STC_MEMBER_INFO either inserts STC_M1 or STC_M2, depending whether it has one or two arguments.
#define STC_MEMBER_INFO_SELECT(_0, _1, macro, ...) macro
#define STC_MEMBER_INFO(type, ...) STC_EXPAND(STC_MEMBER_INFO_SELECT(__VA_ARGS__, STC_M2, STC_M1)(type, __VA_ARGS__))

//STC_CLASS_INFO inserts multiple STC_C1, depending on the number of arguments.
//STC_MEMBER_INFO_WRAP inserts STC_MEMBER_INFO(type, ...) by using STC_PAREN to insert parentheses for making a macro call
#define STC_MEMBER_INFO_WRAP(type, member) STC_MEMBER_INFO STC_PAREN(type, member)
#define STC_C0(type)
#define STC_C1(type, member) STC_MEMBER_INFO_WRAP(type, member)
#define STC_C2(type, member, ...) STC_MEMBER_INFO_WRAP(type, member),STC_EXPAND(STC_C1(type, __VA_ARGS__))
#define STC_C3(type, member, ...) STC_MEMBER_INFO_WRAP(type, member),STC_EXPAND(STC_C2(type, __VA_ARGS__))
#define STC_C4(type, member, ...) STC_MEMBER_INFO_WRAP(type, member),STC_EXPAND(STC_C3(type, __VA_ARGS__))
#define STC_C5(type, member, ...) STC_MEMBER_INFO_WRAP(type, member),STC_EXPAND(STC_C4(type, __VA_ARGS__))
#define STC_C6(type, member, ...) STC_MEMBER_INFO_WRAP(type, member),STC_EXPAND(STC_C5(type, __VA_ARGS__))
#define STC_C7(type, member, ...) STC_MEMBER_INFO_WRAP(type, member),STC_EXPAND(STC_C6(type, __VA_ARGS__))
#define STC_C8(type, member, ...) STC_MEMBER_INFO_WRAP(type, member),STC_EXPAND(STC_C7(type, __VA_ARGS__))
#define STC_C9(type, member, ...) STC_MEMBER_INFO_WRAP(type, member),STC_EXPAND(STC_C8(type, __VA_ARGS__))
#define STC_C10(type, member, ...) STC_MEMBER_INFO_WRAP(type, member),STC_EXPAND(STC_C9(type, __VA_ARGS__))
#define STC_C11(type, member, ...) STC_MEMBER_INFO_WRAP(type, member),STC_EXPAND(STC_C10(type, __VA_ARGS__))
#define STC_C12(type, member, ...) STC_MEMBER_INFO_WRAP(type, member),STC_EXPAND(STC_C11(type, __VA_ARGS__))
#define STC_C13(type, member, ...) STC_MEMBER_INFO_WRAP(type, member),STC_EXPAND(STC_C12(type, __VA_ARGS__))
#define STC_C14(type, member, ...) STC_MEMBER_INFO_WRAP(type, member),STC_EXPAND(STC_C13(type, __VA_ARGS__))
#define STC_C15(type, member, ...) STC_MEMBER_INFO_WRAP(type, member),STC_EXPAND(STC_C14(type, __VA_ARGS__))

#define STC_CLASS_MEMBERS_SELECT(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15, macro, ...) macro 
#define STC_CLASS_MEMBERS(type, ...) STC_EXPAND(STC_CLASS_MEMBERS_SELECT(_0,__VA_ARGS__,STC_C15,STC_C14,STC_C13,STC_C12,STC_C11,STC_C10,STC_C9,STC_C8,STC_C7,STC_C6,STC_C5,STC_C4,STC_C3,STC_C2,STC_C1,STC_C0)(type,__VA_ARGS__))


#else //now the same without STC_EXPAND


#define STC_MEMBER_INFO_SELECT(_0, _1, macro, ...) macro
#define STC_MEMBER_INFO(type, ...) STC_MEMBER_INFO_SELECT(__VA_ARGS__, STC_M2, STC_M1)(type, __VA_ARGS__)

#define STC_MEMBER_INFO_WRAP(type, member) STC_MEMBER_INFO STC_PAREN(type, member)
#define STC_C0(type)
#define STC_C1(type, member) STC_MEMBER_INFO_WRAP(type, member)
#define STC_C2(type, member, ...) STC_MEMBER_INFO_WRAP(type, member),STC_C1(type, __VA_ARGS__)
#define STC_C3(type, member, ...) STC_MEMBER_INFO_WRAP(type, member),STC_C2(type, __VA_ARGS__)
#define STC_C4(type, member, ...) STC_MEMBER_INFO_WRAP(type, member),STC_C3(type, __VA_ARGS__)
#define STC_C5(type, member, ...) STC_MEMBER_INFO_WRAP(type, member),STC_C4(type, __VA_ARGS__)
#define STC_C6(type, member, ...) STC_MEMBER_INFO_WRAP(type, member),STC_C5(type, __VA_ARGS__)
#define STC_C7(type, member, ...) STC_MEMBER_INFO_WRAP(type, member),STC_C6(type, __VA_ARGS__)
#define STC_C8(type, member, ...) STC_MEMBER_INFO_WRAP(type, member),STC_C7(type, __VA_ARGS__)
#define STC_C9(type, member, ...) STC_MEMBER_INFO_WRAP(type, member),STC_C8(type, __VA_ARGS__)
#define STC_C10(type, member, ...) STC_MEMBER_INFO_WRAP(type, member),STC_C9(type, __VA_ARGS__)
#define STC_C11(type, member, ...) STC_MEMBER_INFO_WRAP(type, member),STC_C10(type, __VA_ARGS__)
#define STC_C12(type, member, ...) STC_MEMBER_INFO_WRAP(type, member),STC_C11(type, __VA_ARGS__)
#define STC_C13(type, member, ...) STC_MEMBER_INFO_WRAP(type, member),STC_C12(type, __VA_ARGS__)
#define STC_C14(type, member, ...) STC_MEMBER_INFO_WRAP(type, member),STC_C13(type, __VA_ARGS__)
#define STC_C15(type, member, ...) STC_MEMBER_INFO_WRAP(type, member),STC_C14(type, __VA_ARGS__)

#define STC_CLASS_MEMBERS_SELECT(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15, macro, ...) macro 
#define STC_CLASS_MEMBERS(type, ...) STC_CLASS_MEMBERS_SELECT(_0,__VA_ARGS__,STC_C15,STC_C14,STC_C13,STC_C12,STC_C11,STC_C10,STC_C9,STC_C8,STC_C7,STC_C6,STC_C5,STC_C4,STC_C3,STC_C2,STC_C1,STC_C0)(type,__VA_ARGS__)


#endif

/// \def stc_declare_class
/// Declares class information for \p type by defining either a method or a free function.
/// Variadic arguments may either be of form <member> or (<member>, <member options>).
/// The former declares a member without any options.
#define stc_declare_class(type, ...) static constexpr auto stc_class_info(::stc::type_wrap<type>){ return ::stc::detail::make_class_info<type>( STC_EXPAND(STC_CLASS_MEMBERS(type, __VA_ARGS__)) ); }

}
