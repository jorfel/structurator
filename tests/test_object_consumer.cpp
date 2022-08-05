#include <catch2/catch.hpp>

#include <structurator/class_info.hpp>
#include <structurator/json_input.hpp>
#include <structurator/any_consumer.hpp>
#include <structurator/object_mapper.hpp>
#include <structurator/stdlib_consumers.hpp>

#include <structurator/size_bounded.hpp>
#include <structurator/range_bounded.hpp>


struct A
{
    int alice = 0;
    int bob = 0;
    float claude = 0.0;
};

struct B
{
    int m1 = 0;
    int m2 = 0;

    stc_declare_class(B, m1, m2);
};

struct Complex
{
    std::int32_t int32;
    char ch;
    stc::range_bounded<int, 1, 10> bounded;
    stc::size_bounded<std::string, 1, 10> bounded_string;
    std::unique_ptr<A> unique_ptr;
    std::optional<A> optional;
    std::vector<int> vector;
    std::array<int, 3> array;
    std::map<std::string, int> map;

    std::vector<int> multiple;

    A subobject;

    std::variant<int, std::string> variant1;
    std::variant<int, B> variant2;
    
    std::map<std::string, std::any> additional;
};


stc_declare_class(A,
    alice,
    (bob, stc::member_short("b") | stc::member_alias("Bob") | stc::member_flag::maybe_default),
    claude
);


stc_declare_class(Complex,
    int32,
    ch,
    bounded,
    (bounded_string, stc::member_alias("bounded string")),
    (unique_ptr, stc::member_flag::maybe_default),
    (optional, stc::member_flag::maybe_default),
    vector,
    array,
    map,
    (multiple, stc::member_flag::multiple),
    subobject,
    (
        variant1,
        stc::member_alts("kind1", stc::alt_mode::nest,
            stc::alt<int>("number"), 
            stc::alt<std::string>("text"))
    ),
    (
        variant2,
        stc::member_alts("kind2", stc::alt_mode::no_nesting,
            stc::alt<int>("number"),
            stc::alt<B>("B"))
    ),
    (additional, stc::member_flag::additional_keys)
);


TEST_CASE("Mapper")
{
    SECTION("Class information")
    {
        constexpr auto info = stc::get_class_info<A>();
        constexpr auto member_name1 = std::get<0>(info.members).name;
        REQUIRE(member_name1 == "alice");

        constexpr auto minfo1 = std::get<1>(info.members);

        constexpr auto member_name2 = minfo1.name;
        REQUIRE(member_name2 == "bob");

        constexpr auto member_ptr = minfo1.member_ptr;
        REQUIRE(&A::bob == member_ptr);

        constexpr auto flags = minfo1.options.flags;
        REQUIRE(flags & unsigned(stc::member_flag::maybe_default));

        constexpr auto short_name = stc::get_member_attr<stc::member_short_tag>(minfo1.options).short_name;
        REQUIRE(short_name == "b");

        constexpr auto alias_name = stc::get_member_attr<stc::member_alias_tag>(minfo1.options).alias_name;
        REQUIRE(alias_name == "Bob");
    }
    SECTION("Complex object")
    {
        std::string_view sample = R"(
        {
            "int32": -2e5,
            "ch": "A",
            "bounded": 2,
            "bounded string": "abc",
            "optional" : { "alice": 4, "b": 5, "claude": -6 },
            "vector" : [ 1, 2, 3 ],
            "array": [1, 2, 3],
            "map": { "a": 1, "b": 2 },
            
            "multiple": 0,
            "multiple": 1,
            "multiple": 2,

            "subobject" : { "alice": 4, "b": 5, "claude": 6.25e3 },

            "additional1": 1233,
            "additional2": [],

            "kind1": "text",
            "variant1": "texttext",

            "kind2": "B",
            "m1": 1,
            "m2": 2
        })";
        std::unique_ptr<stc::doc_input> input = stc::json::input(sample, [](const stc::json::parse_error&)
        {
            FAIL();
        });

        std::optional<Complex> c = stc::from_input<Complex>(*input, [](const stc::doc_error&)
        {
            FAIL();
        });

        REQUIRE(c.has_value());
        REQUIRE(c->int32 == -200000);
        REQUIRE(c->ch == 'A');
        REQUIRE(c->bounded == 2);
        REQUIRE((std::string)c->bounded_string == "abc");
        REQUIRE(c->unique_ptr == nullptr);
        REQUIRE(c->optional.has_value());
        REQUIRE(c->optional->claude == -6);
        REQUIRE(c->vector == std::vector<int>{ 1, 2, 3 });
        REQUIRE(c->array == std::array<int, 3>{ 1, 2, 3 });
        REQUIRE(c->map == std::map<std::string, int>{ {"a", 1}, { "b", 2 }});

        REQUIRE(c->multiple == std::vector<int>{ 0, 1, 2 });

        REQUIRE(c->subobject.alice == 4);
        REQUIRE(c->subobject.bob == 5);
        REQUIRE(c->subobject.claude == 6250.0f);

        REQUIRE(std::any_cast<double>(c->additional["additional1"]) == 1233.0);
        REQUIRE(std::any_cast<std::vector<std::any>>(c->additional["additional2"]).empty());

        REQUIRE(c->variant1.index() == 1);
        REQUIRE(std::get<1>(c->variant1) == "texttext");

        REQUIRE(c->variant2.index() == 1);
        REQUIRE(std::get<1>(c->variant2).m1 == 1);
        REQUIRE(std::get<1>(c->variant2).m2 == 2);
    }
}
