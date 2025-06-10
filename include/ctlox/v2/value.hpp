#pragma once

#include <ctlox/v2/function.hpp>
#include <ctlox/v2/literal.hpp>

#include <string>
#include <variant>

namespace ctlox::v2 {

class value_t final {
    using variant_t = std::variant<nil_t, bool, double, std::string, function>;

    variant_t value_;

public:
    template <typename... Args>
        requires std::constructible_from<variant_t, Args&&...>
    constexpr explicit(false) value_t(Args&&... args) noexcept  // NOLINT(*-explicit-constructor)
        : value_(std::forward<Args>(args)...) { }

    constexpr value_t() noexcept = default;

    constexpr value_t(const value_t&) noexcept = default;
    constexpr value_t& operator=(const value_t&) noexcept = default;
    constexpr value_t(value_t&&) noexcept = default;
    constexpr value_t& operator=(value_t&&) noexcept = default;

    template <typename Visitor>
    constexpr auto visit(this auto&& self, Visitor&& v) {
        return std::visit(std::forward<Visitor>(v), std::forward_like<decltype(self)>(self.value_));
    }

    template <typename Value>
    [[nodiscard]] constexpr bool holds() const noexcept {
        return std::holds_alternative<Value>(value_);
    }

    template <typename Value>
    [[nodiscard]] constexpr auto get_if(this auto&& self) noexcept {
        return std::get_if<Value>(&std::forward_like<decltype(self)>(self.value_));
    }

    constexpr bool operator==(const value_t& other) const noexcept = default;
};

}  // namespace ctlox::v2

#include <ctlox/v2/function_impl.hpp>