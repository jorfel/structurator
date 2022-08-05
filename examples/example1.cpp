#include <string>
#include <memory>
#include <cassert>
#include <optional>
#include <string_view>
#include <structurator/json_input.hpp>
#include <structurator/object_mapper.hpp>

struct my_class
{
    int a;
    std::string b;
};

stc_declare_class(my_class, a, b);

int main()
{
    std::string_view json_text = R"(
        {
            "a": 1,
            "b": "text"
        }
    )";

    auto on_parse_error = [](const stc::json::parse_error&) {};
    std::unique_ptr<stc::doc_input> input = stc::json::input(json_text, on_parse_error);

    auto on_consume_error = [](const stc::doc_error&) {};
    std::optional<my_class> my_object = stc::from_input<my_class>(*input, on_consume_error);

    assert(my_object.has_value());
    assert(my_object->a == 1);
    assert(my_object->b == "text");
}