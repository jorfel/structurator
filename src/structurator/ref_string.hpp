#pragma once

#include <memory>
#include <utility>
#include <climits>
#include <cstring>
#include <cassert>
#include <string_view>

namespace stc
{

/// Holds a read-only string that is either owned or non-owned.
/// Owned strings are allocated and non-owned ones are only referenced.
/// This is advantageous in situation where large strings need to be passed. In most cases, these can be
/// passed without copying them, but sometimes they require, for instance in JSON, to be escaped first, so
/// an un-escaped version has to be constructed.
class ref_string
{
public:
    ref_string() = default;

    /// Constructs a non-owning ref_string.
    ref_string(std::string_view source) noexcept : begin(source.data()), length(source.size())
    {
        assert(length < allocated_bit);
    }

    /// Move constructor. No copying allowed.
    ref_string(ref_string &&rhs) noexcept
    {
        *this = std::move(rhs);
    }

    ~ref_string()
    {
        if(is_allocated())
            delete[] begin;
    }

    /// Move assignment.
    ref_string &operator=(ref_string &&rhs) noexcept
    {
        if(is_allocated())
            delete[] begin;

        begin = std::exchange(rhs.begin, nullptr);
        length = std::exchange(rhs.length, 0);
        return *this;
    }

    /// Returns whether the string is owned.
    bool is_allocated() const
    {
        return length & allocated_bit;
    }

    /// When owned, releases the allocated string as a std::unique_ptr.
    std::unique_ptr<char[]> release()
    {
        assert(is_allocated());
        const char *str = std::exchange(begin, nullptr);
        length = 0;
        return std::unique_ptr<char[]>(const_cast<char*>(str));
    }

    /// Pointer to first character or nullptr when empty.
    const char *data() const
    {
        return begin;
    }

    /// Number of characters in the string.
    size_t size() const
    {
        return length & ~allocated_bit;
    }

    bool operator==(const ref_string &rhs) const
    {
        return std::string_view(*this) == std::string_view(rhs);
    }

    bool operator<(const ref_string &rhs) const
    {
        return std::string_view(*this) < std::string_view(rhs);
    }

    operator std::string_view() const
    {
        return std::string_view(data(), size());
    }

    /// Copies the given string into a new owning ref_string.
    static ref_string make_copy(std::string_view source)
    {
        assert(source.size() < allocated_bit);

        char *ptr = new char[source.size()];
        std::memcpy(ptr, source.data(), source.size());

        ref_string str;
        str.begin = ptr;
        str.length = source.size() | allocated_bit;
        return str;
    }

private:
    const char *begin = nullptr;
    size_t length = 0; ///< Highest bit indicates whether the string is referenced or allocated.

    static constexpr size_t allocated_bit = size_t(1) << (sizeof(size_t) * CHAR_BIT - 1);
};


}