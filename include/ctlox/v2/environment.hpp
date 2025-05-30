#pragma once

#include <ctlox/v2/runtime_error.hpp>
#include <ctlox/v2/types.hpp>

#include <utility>
#include <vector>

namespace ctlox::v2 {

class environment {
public:
    constexpr environment(environment* enclosing = nullptr)
        : enclosing_(enclosing) { }

    constexpr value_t get(const token_t& name) const {
        auto it = std::ranges::find(values_, name.lexeme_, &entry_t::first);
        if (it != values_.end()) {
            return it->second;
        }

        if (enclosing_)
            return enclosing_->get(name);

        throw runtime_error(name, "Undefined variable.");
    }

    constexpr void define(const token_t& name, const value_t& value) { values_.emplace_back(name.lexeme_, value); }

    constexpr void assign(const token_t& name, const value_t& value) {
        auto it = std::ranges::find(values_, name.lexeme_, &entry_t::first);
        if (it != values_.end()) {
            it->second = value;
            return;
        }

        if (enclosing_) {
            enclosing_->assign(name, value);
            return;
        }

        throw runtime_error(name, "Undefined variable.");
    }

private:
    using entry_t = std::pair<std::string, value_t>;

    environment* enclosing_ = nullptr;
    std::vector<std::pair<std::string, value_t>> values_;
};

}  // namespace ctlox::v2