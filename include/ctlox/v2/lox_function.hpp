#pragma once

#include <ctlox/v2/program_state.hpp>
#include <ctlox/v2/token.hpp>
#include <ctlox/v2/value.hpp>

#include <cassert>
#include <ranges>
#include <span>
#include <string_view>

namespace ctlox::v2 {

template <const std::string_view& name_, const token_t* params_data_, std::size_t params_size_, typename Body>
class lox_function {
    static constexpr auto params_ = std::span(params_data_, params_size_);

public:
    constexpr lox_function(environment* closure)
        : closure_(closure) { }

    [[nodiscard]] constexpr std::string name() const noexcept { return std::string(name_); }
    [[nodiscard]] constexpr int arity() const noexcept { return params_.size(); }

    constexpr value_t operator()(const program_state_t& state, std::span<const value_t> arguments) const {
        assert(arguments.size() == arity());
        // TODO
        // environment env(closure_);
        auto& env = state.environments_->emplace_back(std::make_unique<environment>(closure_));
        for (const auto& [param, argument] : std::views::zip(params_, arguments)) {
            env->define(param.lexeme_, argument);
        }

        return_slot return_slot;
        const program_state_t function_state = state.with(env.get()).with(&return_slot);

        Body {}(function_state);
        return std::move(return_slot)();
    }

private:
    environment* closure_ = nullptr;
};

}  // namespace ctlox::v2