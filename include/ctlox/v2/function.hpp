#pragma once

#include <format>
#include <memory>
#include <span>

namespace ctlox::v2 {

class value_t;
struct program_state_t;

class _function_impl_base;

class function final {
public:
    constexpr function() = default;
    constexpr ~function() noexcept;

    template <typename Callable>
    constexpr explicit function(Callable&& callable);

    constexpr function(const function& other);
    constexpr function& operator=(const function& other);
    constexpr function(function&& other) = default;
    constexpr function& operator=(function&& other) = default;

    friend constexpr void swap(function& lhs, function& rhs) noexcept { std::ranges::swap(lhs.impl_, rhs.impl_); }

    [[nodiscard]] constexpr bool operator==(const function& other) const noexcept = default;

    [[nodiscard]] constexpr std::string name() const;
    [[nodiscard]] constexpr int arity() const;
    constexpr value_t operator()(const program_state_t& state, std::span<const value_t> arguments) const;


private:
    std::unique_ptr<_function_impl_base> impl_;
};

}  // namespace ctlox::v2

template <>
struct std::formatter<ctlox::v2::function> : std::formatter<std::string> {
    auto format(const ctlox::v2::function& function, std::format_context& ctx) const {
        return std::formatter<std::string>::format(std::format("<fn {}>", function.name()), ctx);
    }
};