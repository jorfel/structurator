#include <string>
#include <memory>
#include <variant>
#include <cstddef>
#include <cassert>
#include <optional>
#include <string_view>
#include <structurator/json_input.hpp>
#include <structurator/object_mapper.hpp>


struct write_entry
{
    std::string new_content;
};

struct delete_entry
{
    bool immediatley;
};

struct log_entry
{
    std::string file_name;
    std::string author;
    std::uint64_t timestamp;

    std::variant<write_entry, delete_entry> payload;
};

stc_declare_class(write_entry, new_content);
stc_declare_class(delete_entry, immediatley);
stc_declare_class(log_entry,
    file_name,
    author,
    timestamp,
    (payload, stc::member_alts("type", stc::alt_mode::nest, stc::alt<write_entry>("write"),
            stc::alt<delete_entry>("delete")))
);


int main()
{
    std::string_view json_text = R"(
        {
            "file_name": "README.md", "author": "Ben", "timestamp": 1234,
            "type": "write",
            "payload": { "new_content": "hello there" }
        }
    )";

    auto on_parse_error = [](const stc::json::parse_error &) {};
    auto input = stc::json::input(json_text, on_parse_error);

    auto on_consume_error = [](const stc::doc_error&) {};
    auto entry = stc::from_input<log_entry>(*input, on_consume_error);

    assert(entry.has_value());
    assert(entry->file_name == "README.md");
    assert(entry->author == "Ben");
    assert(entry->timestamp == 1234);
    assert(entry->payload.index() == 0);
    
    const write_entry &payload = std::get<0>(entry->payload);
    assert(payload.new_content == "hello there");
}