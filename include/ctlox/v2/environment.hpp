#pragma once

#include "native_function.hpp"

#include <ctlox/v2/exception.hpp>
#include <ctlox/v2/value.hpp>

#include <utility>
#include <vector>

namespace ctlox::v2 {

class environment {
public:
    constexpr explicit environment(environment* enclosing = nullptr)
        : enclosing_(enclosing) { }

    constexpr auto ancestor(this auto&& self, int env_depth) noexcept -> decltype(&self) {
        auto* ancestor = &self;
        for (int i = 0; i < env_depth; ++i)
            ancestor = ancestor->enclosing_;
        return ancestor;
    }

    [[nodiscard]] constexpr value_t get(const token_t& name) const {
        // non-capturing lambda to avoid unintentionally accessing this
        auto do_get = [](const environment* env, const token_t& name) {
            for (; env != nullptr; env = env->enclosing_) {
                auto it = std::ranges::find(env->values_, name.lexeme_, &entry_t::first);
                if (it != env->values_.end()) {
                    return it->second;
                }
            }

            throw runtime_error(name, "Undefined variable.");
        };

        return do_get(this, name);
    }

    [[nodiscard]] constexpr value_t get_at(int env_depth, int env_index) const {
        return ancestor(env_depth)->values_[env_index].second;
    }

    constexpr void define(std::string_view name, const value_t& value) { values_.emplace_back(name, value); }

    constexpr void assign(const token_t& name, const value_t& value) {
        // non-capturing lambda to avoid unintentionally accessing this
        auto do_assign = [](environment* env, const token_t& name, const value_t& value) {
            for (; env != nullptr; env = env->enclosing_) {
                auto it = std::ranges::find(env->values_, name.lexeme_, &entry_t::first);
                if (it != env->values_.end()) {
                    it->second = value;
                    return;
                }
            }

            throw runtime_error(name, "Undefined variable.");
        };

        do_assign(this, name, value);
    }

    constexpr void assign_at(int env_depth, int env_index, const value_t& value) {
        ancestor(env_depth)->values_[env_index].second = value;
    }

    template <int arity>
    constexpr void define_native(std::string_view name, auto&& fn) {
        using Fn = decltype(fn);
        using NativeFn = native_function<arity, std::decay_t<Fn>>;
        values_.emplace_back(name, function(NativeFn(name, std::forward<Fn>(fn))));
    }

private:
    using entry_t = std::pair<std::string, value_t>;

    environment* enclosing_ = nullptr;
    std::vector<entry_t> values_;
};

}  // namespace ctlox::v2