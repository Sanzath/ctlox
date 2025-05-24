#pragma once

#include <algorithm>
#include <array>
#include <string_view>

namespace ctlox::inline common {
template <std::size_t N>
struct string {
    constexpr string() = default;

    constexpr string(const char (&str)[N + 1])
    {
        if constexpr (N != 0) {
            std::ranges::copy_n(str, N, str_.begin());
        }
    }

    constexpr string(std::string_view str)
    {
        if constexpr (N != 0) {
            std::ranges::copy_n(str.begin(), N, str_.begin());
        }
    }

    constexpr auto size() const { return N; };
    [[nodiscard]] constexpr bool empty() const { return N == 0; };

    constexpr auto begin() { return str_.begin(); };
    constexpr auto begin() const { return str_.begin(); };
    constexpr auto end() { return str_.end(); };
    constexpr auto end() const { return str_.end(); };

    constexpr char operator[](std::size_t idx) const { return str_[idx]; };

    constexpr operator std::string_view() const
    {
        return std::string_view(str_.begin(), str_.end());
    }

    template <std::size_t first, std::size_t last>
    constexpr string<last - first> substr() const
    {
        string<last - first> other;
        std::ranges::copy(str_.begin() + first, str_.begin() + last, other.begin());
        return other;
    }

    [[nodiscard]] constexpr std::size_t find_next(std::size_t start, char c) const
    {
        const auto iter = std::ranges::find(str_.begin() + start, str_.end(), c);
        return iter == str_.end() ? str_.size() : iter - str_.begin();
    }

    constexpr auto operator<=>(const string&) const = default;
    constexpr bool operator==(const string&) const = default;

    template <std::size_t M>
        requires(N != M)
    constexpr bool operator==(const string<M>&) const
    {
        return false;
    }

    std::array<char, N> str_ = {};
};

template <std::size_t N>
string(const char (&str)[N]) -> string<N - 1>;

inline namespace literals {
    template <string s>
    constexpr auto operator""_ct() { return s; }
}  // namespace literals

template <std::size_t... Ns>
constexpr auto concat(string<Ns> const&... strings)
{
    constexpr std::size_t N = (Ns + ...);
    string<N> out_str;
    auto out_it = out_str.begin();

    auto do_copy = [&](const auto& str) {
        std::ranges::copy(str, out_it);
        std::advance(out_it, str.size());
    };

    (do_copy(strings), ...);

    return out_str;
}
}  // namespace ctlox::inline common