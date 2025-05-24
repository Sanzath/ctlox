#pragma once

#include <variant>
#include <string_view>

namespace ctlox::v2 {

struct nil_t {
    constexpr bool operator==(const nil_t&) const noexcept { return true; }
};
struct none_t {
    constexpr bool operator==(const none_t&) const noexcept { return true; }
};

constexpr inline none_t none;  // not a literal
constexpr inline nil_t nil;  // nil literal

// TODO: string_view or string?
using literal_t = std::variant<none_t, nil_t, bool, double, std::string_view>;

}