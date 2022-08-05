#pragma once

///
/// \file
/// \brief Defines consume() for reading arbitrary classes from documents.
///

#include <tuple>
#include <array>
#include <variant>
#include <utility>
#include <algorithm>

#include "meta.hpp"
#include "doc_input.hpp"
#include "class_info.hpp"
#include "ref_string.hpp"
#include "doc_consumer.hpp"


#pragma warning(push)
#pragma warning(disable: 4068) //MSVC warns about unknown pragmas
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value" //due to fold-expressions with side-effects


namespace stc
{

namespace detail
{


/// Returns a special tuple when \p member_alts is actually an instance of member_alts<...>
/// or not_present when not.
/// The tuple contains a reference to the parameterr, an useful index_sequence for later and and index.
/// The index will later indicate the concrete alternative type.
template<class MemberAlts>
auto make_discriminator_info(const MemberAlts &member_alts)
{
    if constexpr(!std::is_same_v<MemberAlts, not_present_t>)
    {
        using seq = std::make_index_sequence<MemberAlts::alts_count>;
        return std::tuple<const MemberAlts&, seq, size_t>(member_alts, seq(), size_t(-1));
    }
    else
    {
        return not_present;
    }
}

/// Consumes the concrete alternative type.
/// Given a tuple as specified at make_discriminator_info(), uses the alternative type which is dynamically indicated
/// by the index (third entry) and reads content from the document into the given \p target.
template<class T, class DiscrType, class... AltTypes, size_t... AltTypesIdx>
bool consume_discriminated(
    T &target,
    std::tuple<const member_alts<DiscrType, AltTypes...>&, std::index_sequence<AltTypesIdx...>, size_t> &discr_info,
    doc_input::token_kind first, doc_input &input, const doc_context &context)
{
    using stc::consume;
    return (... || ( //iterate all alternatives
        std::get<2>(discr_info) == AltTypesIdx ? //index matches
        target = consume(type_wrap<AltTypes>(), first, input, context), true : //consume into target
        false //next alternative
        ));
}

enum class alt_stat
{
    success,
    skipped,
    consumed_remaining,
    unknown_alternative
};


/// Tries to match a key with values to determine the concrete alternative type.
/// Given a tuple as specified at make_discriminator_info(), tries to match the specified key and reads the discriminative value
/// from the document.
/// Then sets the third value of tuple to the index of the alternative type indicated by the discriminating value or
/// directly reads content from the document, depending on the nesting mode.
/// 
template<class T, class MemberInfo, class DiscrType, class... AltTypes, size_t... AltTypesIdx>
alt_stat try_consume_discriminator(
    std::string_view key,
    T &object,
    const MemberInfo &minfo,
    bool &member_found,
    std::tuple<const member_alts<DiscrType, AltTypes...>&, std::index_sequence<AltTypesIdx...>, size_t> &discr_info,
    doc_input::token_kind first, doc_input &input, const doc_context &context)
{
    const auto &member_alts = std::get<0>(discr_info);
    if(key != member_alts.key)
        return alt_stat::skipped;
        
    //reads std::string_view as ref_string, because former has no sensible consume() function
    using parse_type = std::conditional_t<std::is_same_v<DiscrType, std::string_view>, ref_string, DiscrType>;
    auto value = consume(type_wrap<parse_type>(), first, input, context);

    if(member_alts.mode == alt_mode::nest)
    {
        //iterate all alternatives, record index when matching and short-circuit the disjunction with true
        bool matched = (... || (
            DiscrType(value) == std::get<AltTypesIdx>(member_alts.alternatives).discriminative ? //match alternative
            std::get<2>(discr_info) = AltTypesIdx, true : //place index
            false) //try next alternative
        );

        return matched ? alt_stat::success : alt_stat::unknown_alternative;
    }
    else
    {
        member_found = true;
        auto &member = object.*(minfo.member_ptr);

        //iterate all alternatives, directly read alternative type into member
        bool matched = (... || (
            DiscrType(value) == std::get<AltTypesIdx>(member_alts.alternatives).discriminative ?
            member = consume(type_wrap<AltTypes>(), doc_input::token_kind::begin_mapping, input, context), true :
            false)
        );

        return matched ? alt_stat::consumed_remaining : alt_stat::unknown_alternative;
    }
}

/// Overrload for non-tuples which were returned by make_discriminator_info().
template<class T, class MemberInfo>
constexpr alt_stat try_consume_discriminator(std::string_view key, T&, const MemberInfo&, bool, not_present_t, doc_input::token_kind, doc_input&, const doc_context&)
{
    return alt_stat::skipped;
}


enum class fill_stat
{
    success,
    key_unknown,
    key_duplicate,
    discriminator_missing,
};


/// Fills a member by consuming its type from the document.
/// Tries matching the \p key with the name of the member within the specified \p object at index \p MemberIndex
/// and consumes it.
template<size_t MemberIndex = 0, class T, class DiscrInfo>
fill_stat try_fill_member(
    std::string_view key,
    T &object,
    bool &found_member,
    DiscrInfo &discr_info,
    doc_input::token_kind first, doc_input &input, const doc_context &context)
{
    using stc::consume;

    static constexpr auto cinfo = get_class_info<T>();
    static constexpr const auto &minfo = std::get<MemberIndex>(cinfo.members);

    auto &member = object.*(minfo.member_ptr);
    using member_type = std::remove_reference_t<decltype(member)>;

    static constexpr auto alias = get_member_attr<member_alias_tag>(minfo.options);
    static constexpr auto shortn = get_member_attr<member_short_tag>(minfo.options);

    bool matching_name = false;
    if constexpr(alias != not_present)
        matching_name = key == alias.alias_name;

    if constexpr(shortn != not_present)
        matching_name |= key == shortn.short_name;
    else
        matching_name |= key == minfo.name;

    if(!matching_name)
        return fill_stat::key_unknown;

    if(found_member) //member already encountered before
    {
        if constexpr(minfo.options.flags & unsigned(member_flag::first_of_multiple))
            return fill_stat::success;

        static constexpr unsigned multiple_flags = unsigned(member_flag::last_of_multiple) | unsigned(member_flag::multiple);
        if constexpr((minfo.options.flags & multiple_flags) == 0)
            return fill_stat::key_duplicate;
    }

    if constexpr((minfo.options.flags & unsigned(member_flag::maybe_default)) != 0)
    {
        if(first == doc_input::token_kind::null) //null treated as if member was not present
            return fill_stat::success;
    }

    found_member = true;

    if constexpr(!std::is_same_v<decltype(discr_info), not_present_t&>) //try to consume the member for which alternatives are set
    {
        return consume_discriminated(member, discr_info, first, input, context) ? fill_stat::success : fill_stat::discriminator_missing;
    }
    else if constexpr((minfo.options.flags & unsigned(member_flag::multiple)) != 0)
    {
        using type = typename std::remove_reference_t<decltype(member)>::value_type;
        member.emplace_back(consume(type_wrap<type>(), first, input, context));
        return fill_stat::success;
    }
    else
    {
        member = consume(type_wrap<member_type>(), first, input, context);
        return fill_stat::success;
    }
}

} //end of detail


/// Reads an entire object, depending on its defined class information.
/// This cannot be put into the detail-namespace as MSVC would crash.
template<class T, size_t... MembersIdx>
T consume_members(std::index_sequence<MembersIdx...>, doc_input::token_kind first, doc_input &input, const doc_context &context)
{
    static constexpr auto cinfo = get_class_info<T>();

    T object;
    std::array<bool, cinfo.members_count> found_members = {}; //indicates for which members keys were found, entries initially false

    static constexpr size_t add_keys_idx = //index of member which receives unknown keys, or -1 when none defined
        std::min({ (std::get<MembersIdx>(cinfo.members).options.flags & unsigned(member_flag::additional_keys) ? MembersIdx : size_t(-1))... });

    auto discr_info = std::tuple( //holds some info per member with defined alternative types
        detail::make_discriminator_info(get_member_attr<member_alts_tag>(std::get<MembersIdx>(cinfo.members).options))..., 0 //add trailing element to avoid tuple copy constructor
    );

    doc_input::token_kind token;
    while((token = input.next_token()) != doc_input::token_kind::end_mapping)
    {
        using detail::fill_stat;
        using detail::alt_stat;

        ref_string key = input.mapping_key();

        //try matching a discriminator key: iterate members until try_consume_discriminator() returns something different than "skipped" (disjunction will short-circuit)
        alt_stat alt_status = alt_stat::skipped;
        (... || (
            (alt_status = detail::try_consume_discriminator(
                key,
                object,
                std::get<MembersIdx>(cinfo.members),
                found_members[MembersIdx], std::get<MembersIdx>(discr_info), token, input, context)) != alt_stat::skipped
        ));

        if(alt_status == alt_stat::success)
        {
            continue; 
        }
        else if(alt_status == alt_stat::consumed_remaining)
        {
            break; //consumed remaining keys -> end member consuming
        }
        else if(alt_status == alt_stat::unknown_alternative)
        {
            context.error_handler(doc_error{ input.location(), doc_error::kind::value_unknown });
            throw doc_consume_exception();
        }

        //try matching a member: iterate members until fill_member returns something different than "key_unknown" (disjunction will short-circuit)
        fill_stat fill_status = fill_stat::key_unknown;
        (... || (
            (fill_status = detail::try_fill_member<MembersIdx>(
                key,
                object,
                found_members[MembersIdx],
                std::get<MembersIdx>(discr_info),
                token, input, context)) != fill_stat::key_unknown
        ));

        if(fill_status == fill_stat::key_unknown || fill_status == fill_stat::key_duplicate)
        {
            if constexpr(add_keys_idx != size_t(-1)) //try putting this key+value into a map
            {
                auto &map_member = object.*(std::get<add_keys_idx>(cinfo.members).member_ptr);
                using key_type = typename std::remove_reference_t<decltype(map_member)>::key_type;
                using value_type = typename std::remove_reference_t<decltype(map_member)>::mapped_type;
                auto value = consume(type_wrap<value_type>(), token, input, context);
                map_member.emplace(std::pair<key_type, value_type>(std::move(key), std::move(value)));
            }
            else
            {
                auto error = fill_status == fill_stat::key_unknown ? doc_error::kind::key_unknown : doc_error::kind::key_duplicate;
                context.error_handler(doc_error{ input.location(doc_input::relative_loc::key), error });
                throw doc_consume_exception();
            }
        }
        else if(fill_status == fill_stat::discriminator_missing)
        {
            context.error_handler(doc_error{ input.location(doc_input::relative_loc::key), doc_error::kind::type_unspecified });
            throw doc_consume_exception();
        }
    }

    //check if all required fields are present; some members don't have to occurr due to their flags
    static constexpr unsigned flags_default = unsigned(member_flag::maybe_default) | unsigned(member_flag::additional_keys);
    bool found_all = (... && (found_members[MembersIdx] || (std::get<MembersIdx>(cinfo.members).options.flags & flags_default) != 0));
    if(!found_all)
    {
        context.error_handler(doc_error{ input.location(), doc_error::kind::key_missing });
        throw doc_consume_exception();
    }

    return object;
}


/// Consumes a class for which class information is defined.
template<class T>
std::enable_if_t<get_class_info<T>() != not_present, T> consume(type_wrap<T>, doc_input::token_kind first, doc_input &input, const doc_context &context)
{
    static_assert(std::is_default_constructible_v<T>, "Objects of this class must be default constructible.");

    if(first != doc_input::token_kind::begin_mapping && !input.hint(doc_input::token_kind::begin_mapping))
    {
        context.error_handler(doc_error{ input.location(), doc_error::kind::type_mismatch });
        throw doc_consume_exception();
    }

    constexpr auto cinfo = get_class_info<T>();
    static_assert(cinfo.members_count > 0, "This class must have at least one member.");

    return consume_members<T>(std::make_index_sequence<cinfo.members_count>(), first, input, context);
}

}

#pragma GCC diagnostic pop
#pragma warning(pop)