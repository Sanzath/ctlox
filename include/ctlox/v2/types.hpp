#pragma once

#include <variant>
#include <string_view>
#include <string>

namespace ctlox::v2 {

struct nil_t {
    constexpr bool operator==(const nil_t&) const noexcept { return true; }
};
struct none_t {
    constexpr bool operator==(const none_t&) const noexcept { return true; }

    template <typename T>
    constexpr bool operator==(const T&) const noexcept { return false; }
};

constexpr inline none_t none;  // not a literal
constexpr inline nil_t nil;  // nil literal

using literal_t = std::variant<none_t, nil_t, bool, double, std::string_view>;

using value_t = std::variant<nil_t, bool, double, std::string>;

}