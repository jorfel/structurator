# Structurator – Reads your C++-objects directly from JSON and other formats

Structurator is a (non-single-header) library which allows to easily map structured file formats to classes. It depends on C++17 and has been tested on compliant versions of GCC, Clang and MSVC. The parsing is fast, as it doesn't build any dynamic structures first.

Features:
- Abitrary nesting of objects without extra code
- Choosing alternative types based on a key's value
- Default values, re-naming and aliases for members
- Validation like range-checking
- Pre-definitions for STL-Containers
- Pre-defined error messages, but you can write your own too

Supported formats:
- JSON
- Your own

All content excluding the Catch2-source is licensed under the [BSD-License](LICENSE.txt).

# Quick start

All examples can be found within `examples/`.

## Example 1 – The basics
For every user-defined class, the macro `stc_declare_class` must be invoked, either outside within the _same namespace_, the _global namespace_ or _inside the class_. Latter has the advanage of accessing private members, former enables you to add support for external classes. In its most basic form, you pass the class' name and its members. Then obtain some input source which implements `doc_input` and use `from_input` to consume the class. The following complete example reads an object from JSON.
```cpp
#include <string>
#include <memory>
#include <cassert>
#include <optional>
#include <string_view>
#include <structurator/json_input.hpp> //stc::json::input
#include <structurator/object_mapper.hpp> //stc::from_input

struct my_class
{
    int a;
    std::string b;
};

stc_declare_class(my_class, a, b); //this is important

int main()
{
    std::string_view json_text = R"(
        {
            "a": 1,
            "b": "text"
        }
    )";

    //make input parser (see example 4 for error handling)
    auto on_parse_error = [](const stc::json::parse_error&) {};
    std::unique_ptr<stc::doc_input> input = stc::json::input(json_text, on_parse_error);

    //commence the consuming
    auto on_consume_error = [](const stc::doc_error&) {};
    std::optional<my_class> my_object = stc::from_input<my_class>(*input, on_consume_error);

    assert(my_object.has_value());
    assert(my_object->a == 1);
    assert(my_object->b == "text");
}
```

### Reading from files
As for now, the inputs don't accept (file-)streams but only `string_view`s. In case you want to parse large files, consider using memory-mapped files, for example with [mio](https://github.com/mandreyel/mio). For moderately sized files, use `read_file("path/to/file")` from `input_utilities.hpp` to read everything from an input stream.

## Example 2 – Member options
Reading behaviour can be altered with flags (`member_flags`) and attributes (`member_*`). To specify them, make the member declaration a pair of the member's name and its options. Combine multiple flags or attributes with the  `|` operator.
```cpp
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
    //key may be missing, key may be named "f"
    (flag, stc::member_flag::maybe_default | stc::member_alias("f")),
    //receives unknown keys
    (numbers, stc::member_flag::additional_keys)
);

stc_declare_class(my_class2,
    objects,
    //collects multiple occurences, keys must be named "opt"
    (options, stc::member_flag::multiple | stc::member_short("opt"))
);

//...
std::string_view my_class2_json = R"(
    {
        "objects": [ { "f": true }, { "one": 1, "two": 2, "three": 3 } ],
        "opt": "medium",
        "opt": "with salami"
    }
)";
//...
```
## Example 3 – Type alternatives
In many documents, the concrete data type to be read is dynamically specified in advance with a special key. The library allows to map values to types, which are then choosen for reading the actual data. These values themselves can be of any type, usually they are integers or strings. There are two modes to choose from when using the `member_alts` attribute:
- `alt_mode::nest`: The data is put under a separate key.
- `alt_mode::no_nesting`: Use the remaining keys. Only useful for classes.
  
The type of the member for which alternatives are set can be of any compatible type, in the simplest case it is either `std::variant` or `std::unique_ptr` with a polymorphic class. For the latter, don't forget that the alternative types must be `std::unique_ptr`s too!
```cpp
using namespace stc;
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
//MSVC's IntelliSense may color this red for some reason
stc_declare_class(log_entry, file_name, author, timestamp, ( payload,
        member_alts("type", alt_mode::nest, //key "type" decides how to proceed
            alt<write_entry>("write"), alt<delete_entry>("delete")) )
);
//...
// note that "type" must appear before "payload", but it need not be immediately before it
std::string_view log_entry_json = R"(
    {
        "file_name": "README.md", "author": "Ben", "timestamp": 1234,
        "type": "write",
        "payload": { "new_content": "hello there" }
    }
)";
/* with alt_mode::no_nesting it should look like this:
    {
        "file_name": "README.md", "author": "Ben", "timestamp": 1234,
        "type": "write",
        "new_content": "hello there"
    }
*/
//...
```
## Example 4 – Error handling and validation
The parsers provide error information and try to detect more syntax errors. In this case, `from_input` returns an empty `optional`. All error objects contain the exact location and a cause in form of an enumeration value. If you don't want to spend time on defining own error messages, there are pre-defined ones available.
```cpp
#define STC_DEFINE_MESSAGES //define before #includes for built-in strings for enums
#include <structurator/simple_errors.hpp> //stc::error_string
///...
std::string_view json_text = u8R"(
    [
        { "name": "Rölf", "height": 180, ???? },
        { "name": "Bert", "height" 170 }
    ]
)";

auto on_parse_error = [&](const stc::json::parse_error &err)
{
    std::cerr << stc::error_string(json_text, err);
};

stc::json::input(json_text, on_parse_error);
//...
/* Output:
Line 3: Expected '"' here.
            { "name": "Rölf", "height": 180, ???? },
---------------------------------------------^
Line 4: Expected ':' here.
            { "name": "Bert", "height" 170 }
---------------------------------------^
*/
```

When reading objects, type-errors, out-of-bounds and other invalid values can occur. There are some pre-defined classes that wrap types and check their values. Note that multiple validation errors within the same document are not reported (yet).
```cpp
#define STC_DEFINE_MESSAGES
#include <structurator/validation.hpp>
#include <structurator/size_bounded.hpp>
#include <structurator/range_bounded.hpp>
#include <structurator/simple_errors.hpp>
//...
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
    stc::size_bounded<std::string, 1, 30> name; //at least one, at most 30 chars
    stc::range_bounded<unsigned int, 30, 300> height; //integer between 30 and 300 inclusive
    stc::validated_type<int, my_validator> custom; //customized
    stc_declare_class(person, name, height, custom);
};
//...
auto on_consume_error = [&](const stc::doc_error &err)
{
    std::cerr << stc::error_string(json_text, err);
};
//...
/* Possible output when calling stc::from_input:
Line 3: Value is too small.
            { "name": "Rölf", "height": 0, "custom": 9 }
----------------------------------------^
*/
```

## Custom inputs
Adding new input sources is done by implementing `doc_input` from `doc_input.hpp`. The interface is fairly generic and must traverse the document depth-first.

## Custome `consume()` functions
In case you want your special class to be readable without using the `stc_declare_class` macro, write a function `consume()` and put it next to your class, so it can be found using argument-dependent lookup:
```cpp
my_class consume(stc::type_wrap<my_class>, stc::doc_input::token_kind first_token, stc::doc_input &input, stc::doc_context &context)
{
    //Use first_token and call input.next_token() to get more tokens.
    //In case of errors, call context.error_handler and raise doc_consume_exception.
}
```
The type of `context` is `doc_context` by default when using `from_input`, but you may derive from it and use your custom context to be passed around with `from_input_with_context`.

## Pre-defined consumers:
These are included with `object_mapper.hpp`:

- In `native_consumers.hpp`:
  - `bool` from boolean (JSON true/false)
  - `(unsigned)` `short`, `int`, `long`, `long long` + `signed char` and `unsigned char` from numbers without fractional digits
  - `float`, `double`, `long double` from numbers with possibly fractional digits
  - `char` from a string with exactly one UTF-8 code-unit
  - `stc::ref_string` from string
- In `stdlib_consumers.hpp`:
    - `std::string` from string
    - `std::optional<T>` from either T or null (JSON null)
    - `std::unique_ptr<T>` from T
    - `std::array<T, N>` from a list of exactly N elements of type T
    - `std::vector<T>` from a list of zero or more T
    - `std::map<K, V>` from key-value mapping of V with K being constructible from `stc::ref_string`
- In `object_consumer.hpp`:
    - Classes T for which the macro `stc_declare_class` was used. This macro basically just defines a function or method `stc_class_info` that returns `stc::class_info`, which then can be used to inspect T.

These must be included manually:

- In `validation.hpp`, `range_bounded.hpp` and `size_bounded.hpp`:
  - `stc::validated_type<T, Validator>` as T
  - `stc::range_bounded<T, Min, Max>` as T
  - `stc::size_bounded<T, Min, Max>` as a container-like T

The type `stc::ref_string` is a simple read-only class that contains either just a view of a non-owned string or an allocated, owned string. It is useful for passing strings around without unneccessarily copying it.

## Limitations
- `stc_declare_class` accepts up to 16 members.
- Classes must be default-constructible.
- Custom validation of entire objects is possible with `validated_type`, but there is no way of getting location information for single members.
- For now, sub-classes must also present all super-members to `stc_declare_class`.
- Only one validation error is reported.

# Building

Using CMake, add the library as a subfolder
```
add_subdirectoy(path/to/CMakeLists.txt)
```

and link the library to your target:
```
target_link_libraries(your-project-target PRIVATE structurator)
```

You can also build and install the library using CMake if you want to permanently share it with other projects on your machine:
```
mkdir build
cd build
cmake -DSTRUCTURATOR_INSTALL=ON ..
make install
g++ examples/example1.cpp --std=c++17 -lstructurator
```

In case you don't use CMake, you can just copy the files from `src/` and compile them with your project.

## Testing
There are some more complex tests within `tests/` that use [Catch2](https://github.com/catchorg/Catch2), of which a copy is already in `extlib/`. Unless the CMakeLists.txt of this library is the main project, you have to enable tests by setting the CMake-variable `STRUCTURATOR_TESTS` to `ON`.

## Examples
Similarily, examples are built when `STRUCTURATOR_EXAMPLES` is `ON`.

# Notes

## Doxygen
Use `doxygen` to build source documentation for the library.

## Unicode
The library uses UTF-8 everywhere. There are a handful of useful utilities in `src/utf8.hpp` for your own usage too. However, unicode parsing is very complex. The provided simple functions don't, for example, correctly ignore zero-width code-points when counting the number of characters, but they are good enough for Latin texts. If you insists on correct handling, you can use [ICU](https://unicode-org.github.io/icu/) and extract information for error messages by yourself using the provided `byte` index of the provided error object.

## Performance considerations
- The library uses `constexpr` and templates extensively, so structure information declared with `stc_declare_class` is not built or evaluated dynamically.
- Documents are not parsed into separate data structures first.
- GCC prior version 11, MSVC prior version 19.24 and Clang don't support `std::from_chars` for floats, so `std::strtof/d/ld` is used, which is slower and might impact performance for documents with lots of floats.
