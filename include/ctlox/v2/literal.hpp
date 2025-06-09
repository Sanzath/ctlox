#pragma once

#include <format>
#include <string_view>
#include <variant>

namespace ctlox::v2 {

struct nil_t {
    constexpr bool operator==(const nil_t&) const noexcept { return true; }
};
struct none_t {
    constexpr bool operator==(const none_t&) const noexcept { return true; }

    template <typename T>
    constexpr bool operator==(const T&) const noexcept {
        return false;
    }
};

constexpr inline none_t none;  // not a literal
constexpr inline nil_t nil;    // nil literal

using literal_t = std::variant<none_t, nil_t, bool, double, std::string_view>;

}  // namespace ctlox::v2

template <>
struct std::formatter<ctlox::v2::nil_t, char> : std::formatter<std::string_view> {
    auto format(ctlox::v2::nil_t, std::format_context& ctx) const {
        return formatter<std::string_view>::format("nil", ctx);
    }
};