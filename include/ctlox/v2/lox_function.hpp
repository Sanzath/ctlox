#pragma once

#include <ctlox/v2/environment.hpp>
#include <ctlox/v2/program_state.hpp>
#include <ctlox/v2/resolver.hpp>
#include <ctlox/v2/token.hpp>
#include <ctlox/v2/value.hpp>

#include <cassert>
#include <ranges>
#include <span>
#include <string_view>

namespace ctlox::v2 {

template <
    const std::string_view& name_,
    span_t<token_t> s_params_,
    span_t<var_index_t> s_closure_upvalues_,
    span_t<var_index_t> s_scope_upvalues_,
    typename Body>
class lox_function {
    static constexpr std::span<const token_t> params_ = s_params_;
    static constexpr std::span<const var_index_t> closure_upvalues_ = s_closure_upvalues_;
    static constexpr std::span<const var_index_t> scope_upvalues_ = s_scope_upvalues_;

public:
    constexpr lox_function(environment* env)
        : closure_(environment::as_closure, env, closure_upvalues_) { }

    [[nodiscard]] constexpr std::string name() const noexcept { return std::string(name_); }
    [[nodiscard]] constexpr int arity() const noexcept { return params_.size(); }

    constexpr value_t operator()(const program_state_t& state, std::span<const value_t> arguments) const {
        assert(arguments.size() == arity());

        environment env(&closure_, state.heap_, scope_upvalues_);
        for (const auto& [param, argument] : std::views::zip(params_, arguments)) {
            env.define(param.lexeme_, argument);
        }

        return_slot return_slot;
        const program_state_t function_state = state.with(&env).with(&return_slot);

        Body {}(function_state);
        return std::move(return_slot)();
    }

private:
    // Closure only holds shared_values and so all accesses will actually
    // go through to the heap.
    mutable environment closure_;
};

}  // namespace ctlox::v2