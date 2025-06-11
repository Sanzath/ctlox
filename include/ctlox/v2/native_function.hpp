#pragma once

#include <ctlox/v2/value.hpp>

#include <concepts>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace ctlox::v2 {

template <int arity_, typename Fn>
class native_function {
public:
    constexpr native_function(std::string_view name, const Fn& fn)
        : name_(name)
        , fn_(fn) { }

    [[nodiscard]] constexpr const std::string& name() const noexcept { return name_; }
    [[nodiscard]] constexpr int arity() const noexcept { return arity_; }

    constexpr value_t operator()(const program_state_t& state, std::span<const value_t> arguments) const {
        using index_sequence = std::make_index_sequence<arity_>;
        return [&]<std::size_t... I>(std::index_sequence<I...>) -> value_t {
            if constexpr (requires { fn_(state, arguments[I]...); }) {
                using result_type = decltype(fn_(state, arguments[I]...));
                if constexpr (std::convertible_to<value_t, result_type>) {
                    return fn_(state, arguments[I]...);
                }

                else if constexpr (std::same_as<void, result_type>) {
                    fn_(state, arguments[I]...);
                    return nil;
                }

                else {
                    static_assert(false, "Native function must return value_t or void.");
                }
            }

            else if constexpr (requires { fn_(arguments[I]...); }) {
                using result_type = decltype(fn_(arguments[I]...));
                if constexpr (std::convertible_to<value_t, result_type>) {
                    return fn_(arguments[I]...);
                }

                else if constexpr (std::same_as<void, result_type>) {
                    fn_(arguments[I]...);
                    return nil;
                }

                else {
                    static_assert(false, "Native function must return value_t or void.");
                }
            }

            else {
                static_assert(
                    false,
                    "Native function must take arguments equal to its arity, and may optionally take a program_state_t "
                    "as its first argument.");
            }
        }(index_sequence {});
    }

private:
    std::string name_;
    Fn fn_;
};

}  // namespace ctlox::v2