#include <map>
#include <vector>
#include <string>
#include <memory>
#include <cassert>
#include <optional>
#include <string_view>
#include <structurator/json_input.hpp>
#include <structurator/object_mapper.hpp>

struct my_class1
{
    bool flag = false;
    std::map<std::string, int> numbers;
};

struct my_class2
{
    std::vector<my_class1> objects;
    std::vector<std::string> options;
};

stc_declare_class(my_class1,
    (flag, stc::member_flag::maybe_default | stc::member_alias("f")),
    (numbers, stc::member_flag::additional_keys)
);

stc_declare_class(my_class2,
    objects,
    (options, stc::member_flag::multiple | stc::member_short("opt"))
);

int main()
{
    std::string_view json_text = R"(
        {
            "objects": [ { "f": true }, { "one": 1, "two": 2, "three": 3 } ],
            "opt": "medium",
            "opt": "with salami"
        }
    )";

    auto on_parse_error = [](const stc::json::parse_error &) {};
    auto input = stc::json::input(json_text, on_parse_error);

    auto on_consume_error = [](const stc::doc_error&) {};
    auto my_object = stc::from_input<my_class2>(*input, on_consume_error);

    assert(my_object.has_value());
    assert(my_object->options == std::vector<std::string>({ "medium", "with salami" }));
    assert(my_object->objects.size() == 2);

    assert(my_object->objects[0].flag == true);
    assert(my_object->objects[0].numbers.empty());

    assert(my_object->objects[1].flag == false);
    assert(my_object->objects[1].numbers["one"] == 1);
    assert(my_object->objects[1].numbers["two"] == 2);
    assert(my_object->objects[1].numbers["three"] == 3);
}