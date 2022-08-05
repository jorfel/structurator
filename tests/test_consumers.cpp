#include <catch2/catch.hpp>

#include <structurator/json_input.hpp>
#include <structurator/native_consumers.hpp>
#include <structurator/stdlib_consumers.hpp>


TEST_CASE("Native consumers")
{
    using namespace stc;

    stc::doc_context common_context;
    common_context.error_handler = [](const stc::doc_error &err)
    {
        FAIL();
    };

    SECTION("Integer")
    {
        auto input = stc::json::input("  3e5", [](const stc::json::parse_error &)
        {
            FAIL();
        });

        int value = consume(stc::type_wrap<int>(), input->next_token(), *input, common_context);
        REQUIRE(value == 300000);
    }
    SECTION("String")
    {
        auto input = stc::json::input("\"string\"", [](const stc::json::parse_error &)
        {
            FAIL();
        });

        std::string value = consume(stc::type_wrap<std::string>(), input->next_token(), *input, common_context);
        REQUIRE(value == "string");
    }
    SECTION("Present optional")
    {
        auto input = stc::json::input(" 1234", [](const stc::json::parse_error &)
        {
            FAIL();
        });

        auto value = consume(stc::type_wrap<std::optional<int>>(), input->next_token(), *input, common_context);
        REQUIRE(value.has_value());
        REQUIRE(*value == 1234);
    }
    SECTION("Empty optional")
    {
        auto input = stc::json::input("", [](const stc::json::parse_error &)
        {
            FAIL();
        });

        auto value = consume(stc::type_wrap<std::optional<int>>(), input->next_token(), *input, common_context);
        REQUIRE(!value.has_value());
    }
}