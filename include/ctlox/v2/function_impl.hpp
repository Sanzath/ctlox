#pragma once

#include <ctlox/v2/function.hpp>
#include <ctlox/v2/value.hpp>

namespace ctlox::v2 {

template <typename Function>
concept callable = requires(const Function& function, const program_state_t& state, const std::span<const value_t>& args) {
    { function.name() } -> std::convertible_to<std::string>;
    { function.arity() } -> std::convertible_to<int>;
    { function(state, args) } -> std::convertible_to<value_t>;
};

class _function_impl_base {
public:
    virtual ~_function_impl_base() = default;

    [[nodiscard]] constexpr virtual std::string name() const = 0;
    [[nodiscard]] constexpr virtual int arity() const = 0;
    constexpr virtual value_t operator()(const program_state_t& state, std::span<const value_t> arguments) const = 0;
    [[nodiscard]] virtual std::unique_ptr<_function_impl_base> clone() const = 0;
};

template <callable Callable>
class _function_impl : public _function_impl_base {
public:
    constexpr _function_impl(const Callable& callable)  // NOLINT(*-explicit-constructor)
        : callable_(callable) { }
    constexpr _function_impl(Callable&& callable)       // NOLINT(*-explicit-constructor)
        : callable_(std::move(callable)) { }

    constexpr _function_impl(const _function_impl& other) = default;
    constexpr _function_impl& operator=(const _function_impl& other) = default;

    [[nodiscard]] constexpr std::string name() const override { return callable_.name(); }
    [[nodiscard]] constexpr int arity() const override { return callable_.arity(); }
    constexpr value_t operator()(const program_state_t& state, std::span<const value_t> arguments) const override {
        return callable_(state, arguments);
    }

    [[nodiscard]] constexpr std::unique_ptr<_function_impl_base> clone() const override {
        return std::make_unique<_function_impl>(*this);
    }

    Callable callable_;
};

constexpr function::~function() noexcept = default;

template <typename Callable>
constexpr function::function(Callable&& callable)
    : impl_(std::make_unique<_function_impl<std::decay_t<Callable>>>(std::forward<Callable>(callable))) { }

constexpr function::function(const function& other)
    : impl_(other.impl_->clone()) { }

constexpr function& function::operator=(const function& other) {
    function tmp(other);
    swap(*this, tmp);
    return *this;
}

[[nodiscard]] constexpr std::string function::name() const { return impl_->name(); }
[[nodiscard]] constexpr int function::arity() const { return impl_->arity(); }
constexpr value_t function::operator()(const program_state_t& state, std::span<const value_t> arguments) const {
    return (*impl_)(state, arguments);
}

}  // namespace ctlox::v2