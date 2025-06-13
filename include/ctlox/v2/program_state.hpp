#pragma once

#include <ctlox/v2/value.hpp>

namespace ctlox::v2 {

class environment;

struct break_slot {
    bool active_ = false;
    constexpr void operator()() noexcept { active_ = true; }
    constexpr explicit operator bool() const noexcept { return active_; }
};

struct return_slot {
    value_t value_;
    constexpr void operator()(value_t value) noexcept { value_ = std::move(value); }

    constexpr value_t operator()() && noexcept { return std::move(value_); }
};

struct program_state_t {
    heap_t* heap_ = nullptr;
    environment* globals_ = nullptr;

    environment* env_ = nullptr;
    break_slot* break_slot_ = nullptr;
    return_slot* return_slot_ = nullptr;

    constexpr program_state_t(heap_t* heap, environment* globals)
        : globals_(globals)
        , heap_(heap)
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