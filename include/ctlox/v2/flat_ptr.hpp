#pragma once

#include <iterator>
#include <limits>
#include <stdexcept>

namespace ctlox::v2 {

template <typename T>
struct flat_ptr final {
    std::size_t i = std::numeric_limits<std::size_t>::max();

    constexpr bool operator==(const flat_ptr& other) const noexcept = default;
    constexpr auto operator<=>(const flat_ptr& other) const noexcept = default;
};

struct flat_nullptr_t final {
    template <typename T>
    constexpr operator flat_ptr<T>() const {  // NOLINT(*-explicit-constructor)
        return flat_ptr<T> {};
    }
};

constexpr flat_nullptr_t flat_nullptr;

template <typename T>
struct flat_list final {
    flat_ptr<T> first_;
    flat_ptr<T> last_;

    using value_type = flat_ptr<T>;

    struct const_iterator final {
        using difference_type = std::ptrdiff_t;
        using value_type = const flat_ptr<T>;
        using iterator_concept = std::forward_iterator_tag;

        constexpr value_type operator*() const noexcept { return ptr_; }

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

        flat_ptr<T> ptr_;
    };

    [[nodiscard]] constexpr const_iterator begin() const noexcept { return const_iterator { first_ }; }
    [[nodiscard]] constexpr const_iterator end() const noexcept { return const_iterator { last_ }; }

    [[nodiscard]] constexpr std::size_t size() const noexcept { return last_.i - first_.i; }
    [[nodiscard]] constexpr bool empty() const noexcept { return first_ == last_; }

    [[nodiscard]] constexpr flat_ptr<T> operator[](std::size_t i) const {
        if (i >= size()) {
            throw std::out_of_range("flat_list<T>::operator[]");
        }
        return flat_ptr<T> { first_.i + i };
    }
};

static_assert(std::forward_iterator<typename flat_list<int>::const_iterator>);

}  // namespace ctlox::v2