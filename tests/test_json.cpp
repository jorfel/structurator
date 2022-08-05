#include <catch2/catch.hpp>

#include <structurator/json_input.hpp>
#include "stringify_document.hpp"


TEST_CASE("JSON input")
{
    SECTION("Empty document")
    {
        auto input = stc::json::input("  ", [](const stc::json::parse_error &)
        {
            FAIL();
        });

        REQUIRE(stringify_document(*input) == "<eof>");
    }
    SECTION("Complex document")
    {
        std::string_view sample = R"(
        {
            "n1" :123,
            "n2": 123.0 , 
            "n3": 123e3,
            "n4": 123.0e-3,
            "string": "abc",
            "bool1": true,
            "bool2": false,
            "null": null,
            "": "empty",
            "array": [ {"a":432}, 555, [ ] ]
        }
        )";

        auto input = stc::json::input(sample, [](const stc::json::parse_error &)
        {
            FAIL();
        });

        REQUIRE(stringify_document(*input) == "<map>'n1'=123 'n2'=123.0 'n3'=123e3 'n4'=123.0e-3 'string'="
                                            "'abc''bool1'=true'bool2'=false'null'=null''='empty''array'=<array>entry=<map>"
                                            "'a'=432 </map>entry=555 entry=<array></array></array></map>");
    }
    SECTION("Error handling and recovery")
    {
        std::string_view sample = R"([
        {
            "a" : 456,
            "b" : "no end quote,
            "c" : null
        },
        {
            abc
        }
        )";

        unsigned errcount = 0;
        auto input = stc::json::input(sample, [&](const stc::json::parse_error &err)
        {
            if(errcount == 0)
            {
                REQUIRE(err.location.line == 4);
                REQUIRE(err.what == stc::json::parse_error::kind::string_invalid_newline);
            }
            else if(errcount == 1)
            {
                REQUIRE(err.location.line == 8);
                REQUIRE(err.what == stc::json::parse_error::kind::expected_key);
            }
            else if(errcount == 2)
            {
                REQUIRE(err.location.line == 10);
                REQUIRE(err.what == stc::json::parse_error::kind::eof_unexpected);
            }
            else
            {
                FAIL();
            }

            errcount++;
        });

        try
        {
            stringify_document(*input);
        }
        catch(const stc::doc_input_exception&)
        {
        }

        REQUIRE(errcount == 3);
    }
    SECTION("String escape sequences")
    {
        auto input = stc::json::input(u8R"("abc \t \n\f \\ \z \U123 \U2191 \uD834\uDD1E")", [](const stc::json::parse_error &)
        {
            FAIL();
        });

        REQUIRE(stringify_document(*input) == u8"'abc \t \n\f \\ \\z \\U123 \u2191 \U0001D11E'");
    }
}