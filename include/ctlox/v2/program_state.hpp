#pragma once

#include <ctlox/v2/value.hpp>

#include <concepts>
#include <print>

namespace ctlox::v2 {

class environment;

template <typename Fn>
concept _print_fn = std::invocable<Fn, const value_t&>;

struct default_print_fn {
    static void operator()(const value_t& value) {
        value.visit([](const auto& val) { std::print("{}\n", val); });
    }
};

static_assert(_print_fn<default_print_fn>);

struct break_slot {
    bool active_ = false;
    constexpr void operator()() noexcept { active_ = true; }
    constexpr explicit operator bool() const noexcept { return active_; }
};

struct return_slot { };

struct program_state_t {
    environment* globals_ = nullptr;

    environment* env_ = nullptr;
    break_slot* break_slot_ = nullptr;
    return_slot* return_slot_ = nullptr;

    constexpr program_state_t(environment* env)
        : globals_(env)
        , env_(globals_) { }

    constexpr program_state_t with(environment* env) const {
        program_state_t substate = *this;
        substate.env_ = env;
        return substate;
    }

    constexpr program_state_t with(break_slot* break_slot) const {
        program_state_t substate = *this;
        substate.break_slot_ = break_slot;
        return substate;
    }

    constexpr program_state_t with(return_slot* return_slot) const {
        program_state_t substate = *this;
        substate.return_slot_ = return_slot;
        return substate;
    }
};

}  // namespace ctlox::v2