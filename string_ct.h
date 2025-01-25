#pragma once

#include <array>
#include <algorithm>
#include <string_view>

namespace ctlox {
    template <std::size_t N>
    struct string_ct {
        constexpr string_ct() = default;

        constexpr string_ct(const char(&str)[N + 1]) {
            if constexpr (N != 0) {
                std::ranges::copy_n(str, N, str_.begin());
            }
        }

        constexpr string_ct(std::string_view str) {
            if constexpr (N != 0) {
                std::ranges::copy_n(str.begin(), N, str_.begin());
            }
        }

        constexpr auto size() const { return N; };
        constexpr bool empty() const { return N == 0; };

        constexpr auto begin() { return str_.begin(); };
        constexpr auto begin() const { return str_.begin(); };
        constexpr auto end() { return str_.end(); };
        constexpr auto end() const { return str_.end(); };

        constexpr char operator[](int idx) const { return str_[idx]; };

        constexpr operator std::string_view() const {
            return std::string_view(str_.begin(), str_.end());
        }

        template <int first, int last>
        constexpr string_ct<last - first> substr() const {
            string_ct<last - first> other;
            std::ranges::copy(str_.begin() + first, str_.begin() + last, other.begin());
            return other;
        }

        constexpr int find_next(int start, char c) const {
            const auto iter = std::ranges::find(str_.begin() + start, str_.end(), c);
            return static_cast<int>(iter == str_.end() ? str_.size() : iter - str_.begin());
        }

        constexpr auto operator<=>(const string_ct&) const = default;
        constexpr bool operator==(const string_ct&) const = default;

        std::array<char, N> str_ = {};
    };

    template <std::size_t N>
    string_ct(const char(&str)[N]) -> string_ct<N - 1>;
}