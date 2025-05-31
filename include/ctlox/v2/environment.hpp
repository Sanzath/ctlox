#pragma once

#include <ctlox/v2/exception.hpp>
#include <ctlox/v2/types.hpp>

#include <utility>
#include <vector>

namespace ctlox::v2 {

class environment {
public:
    constexpr explicit environment(environment* enclosing = nullptr)
        : enclosing_(enclosing) { }

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

    constexpr void define(const token_t& name, const value_t& value) { values_.emplace_back(name.lexeme_, value); }

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

private:
    using entry_t = std::pair<std::string, value_t>;

    environment* enclosing_ = nullptr;
    std::vector<std::pair<std::string, value_t>> values_;
};

}  // namespace ctlox::v2