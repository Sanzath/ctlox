#pragma once

#include <cstddef>
#include <iterator>
#include <limits>

namespace ctlox::v2 {

template <typename T>
struct flat_ptr_t final {
    std::size_t i = std::numeric_limits<std::size_t>::max();

    constexpr bool operator==(const flat_ptr_t& other) const noexcept = default;
};

struct flat_nullptr_t final {
    template <typename T>
    constexpr operator flat_ptr_t<T>() const {
        return flat_ptr_t<T> {};
    }
};

constexpr flat_nullptr_t flat_nullptr;

template <typename T>
struct flat_list_t final {
    flat_ptr_t<T> first_;
    flat_ptr_t<T> last_;

    struct const_iterator final {
        using difference_type = std::ptrdiff_t;
        using value_type = const flat_ptr_t<T>;
        using pointer = const flat_ptr_t<T>*;
        using reference = const flat_ptr_t<T>&;
        using iterator_category = std::random_access_iterator_tag;

        constexpr reference operator*() const noexcept { return ptr_; }
        constexpr pointer operator->() const noexcept { return &ptr_; }

        constexpr const_iterator& operator++() noexcept {
            ++ptr_.i;
            return *this;
        }

        constexpr const_iterator operator++(int) noexcept {
            const_iterator tmp = *this;
            ++ptr_.i;
            return tmp;
        }

        constexpr bool operator==(const const_iterator& other) const noexcept = default;

        flat_ptr_t<T> ptr_;
    };

    constexpr const_iterator begin() const noexcept { return const_iterator { first_ }; }
    constexpr const_iterator end() const noexcept { return const_iterator { last_ }; }
};

}  // namespace ctlox::v2