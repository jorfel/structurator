#include <string>
#include <memory>
#include <cassert>
#include <optional>
#include <iostream>
#include <string_view>

#define STC_DEFINE_MESSAGES
#include <structurator/json_input.hpp>
#include <structurator/object_mapper.hpp>

#include <structurator/validation.hpp>
#include <structurator/size_bounded.hpp>
#include <structurator/range_bounded.hpp>
#include <structurator/simple_errors.hpp>

struct my_validator
{
    std::optional<stc::doc_error::kind> operator()(int i) const
    {
        if(i > 10)
            return stc::doc_error::kind::value_too_big;

        return std::nullopt;
    }
};

struct person
{
    stc::size_bounded<std::string, 1> name;
    stc::range_bounded<unsigned int, 30, 300> height;
    stc::validated_type<int, my_validator> custom;
};

stc_declare_class(person, name, height, custom);


int main()
{
    std::string_view json_text1 = u8R"(
        [
            { "name": "Rölf", "height": 180, ???? },
            { "name": "Bert", "height" 170, "custom": 9 }
        ]
    )";

    std::string_view json_text2 = u8R"(
        [
            { "name": "Rölf", "height": 0, "custom": 9 }
        ]
    )";

    auto on_parse_error = [&](const stc::json::parse_error &err)
    {
        std::cerr << stc::error_string(json_text1, err);
    };
    auto on_consume_error = [&](const stc::doc_error &err)
    {
        std::cerr << stc::error_string(json_text2, err);
    };

    std::unique_ptr<stc::doc_input> input = stc::json::input(json_text1, on_parse_error);
    stc::from_input<std::vector<person>>(*input, on_consume_error);

    input = stc::json::input(json_text2, on_parse_error);
    stc::from_input<std::vector<person>>(*input, on_consume_error);
}