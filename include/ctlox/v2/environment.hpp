#pragma once

#include <ctlox/v2/exception.hpp>
#include <ctlox/v2/native_function.hpp>
#include <ctlox/v2/resolver.hpp>
#include <ctlox/v2/value.hpp>

#include <cassert>
#include <utility>
#include <vector>

namespace ctlox::v2 {

struct shared_value_t;

class heap_t {
public:
    constexpr ~heap_t() noexcept;

    constexpr shared_value_t create(const value_t& value);

protected:
    friend shared_value_t;

    constexpr value_t get(int index) const;
    constexpr void assign(int index, const value_t& value);

    constexpr void acquire(int index) noexcept;
    constexpr void release(int index) noexcept;

private:
    struct entry_t {
        value_t value_;
        int count_ = 0;
        int next_index_ = -1;
    };

    std::vector<entry_t> entries_;
    int next_index_ = 0;
    bool destroying_ = false;
};

struct shared_value_t {
    constexpr shared_value_t(heap_t* heap, int index) noexcept
        : heap_(heap)
        , index_(index) {
        // pre-acquired.
    }

    constexpr shared_value_t(const shared_value_t& other) noexcept
        : heap_(other.heap_)
        , index_(other.index_) {
        if (heap_)
            heap_->acquire(index_);
    }

    constexpr shared_value_t& operator=(const shared_value_t& other) noexcept {
        shared_value_t tmp(other);
        swap(*this, tmp);
        return *this;
    }

    constexpr shared_value_t(shared_value_t&& other) noexcept { swap(*this, other); }

    constexpr shared_value_t& operator=(shared_value_t&& other) noexcept {
        shared_value_t tmp(std::move(other));
        swap(*this, tmp);
        return *this;
    }

    constexpr ~shared_value_t() noexcept {
        if (heap_)
            heap_->release(index_);
    }

    constexpr friend void swap(shared_value_t& lhs, shared_value_t& rhs) noexcept {
        using std::swap;
        swap(lhs.heap_, rhs.heap_);
        swap(lhs.index_, rhs.index_);
    }

    constexpr void assign(const value_t& value) {
        assert(heap_);
        heap_->assign(index_, value);
    }

    constexpr value_t get() const {
        assert(heap_);
        return heap_->get(index_);
    };

private:
    heap_t* heap_ = nullptr;
    int index_ = -1;
};

constexpr heap_t::~heap_t() noexcept {
    // The current implementation does not do garbage collection,
    // so any cycles remain alive until the end of the program.
    destroying_ = true;
    entries_.clear();
}

constexpr shared_value_t heap_t::create(const value_t& value) {
    int index = next_index_;
    assert(index >= 0);
    if (index == entries_.size()) {
        entries_.emplace_back(value, 1, -1);
        ++next_index_;
    } else {
        entry_t& entry = entries_[index];
        assert(entry.count_ == 0);
        entry.value_ = value;
        entry.count_ = 1;
        next_index_ = std::exchange(entry.next_index_, -1);
    }

    return shared_value_t(this, index);
}

constexpr value_t heap_t::get(int index) const { return entries_[index].value_; }
constexpr void heap_t::assign(int index, const value_t& value) { entries_[index].value_ = value; }

constexpr void heap_t::acquire(int index) noexcept { entries_[index].count_++; }
constexpr void heap_t::release(int index) noexcept {
    if (destroying_)
        return;

    auto& entry = entries_[index];
    if (--entry.count_ == 0) {
        entry.next_index_ = std::exchange(next_index_, index);

        // Overwrite the value in case it holds upvalues itself.
        entry.value_ = nil;
    }
}

class environment {
public:
    // global environment
    constexpr explicit environment() noexcept = default;

    // non-global environment
    constexpr explicit environment(environment* enclosing, heap_t* heap, std::span<const var_index_t> upvalues)
        : enclosing_(enclosing)
        , heap_(heap)
        , upvalues_(upvalues) { }

    struct as_closure_tag { };
    static constexpr as_closure_tag as_closure;

    // closure
    constexpr explicit environment(
        as_closure_tag, environment* env, std::span<const var_index_t> closure_upvalues) noexcept {

        for (const auto& [env_depth, env_index] : closure_upvalues) {
            const variable_t& variable = env->ancestor(env_depth)->values_[env_index];
            const shared_value_t* upvalue = std::get_if<shared_value_t>(&variable.value_);
            assert(upvalue != nullptr);

            values_.emplace_back(variable.name_, *upvalue);
        }
    }

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
                auto it = std::ranges::find(env->values_, name.lexeme_, &variable_t::name_);
                if (it != env->values_.end()) {
                    return get_impl(*it);
                }
            }

            throw runtime_error(name, "Undefined variable.");
        };

        return do_get(this, name);
    }

    [[nodiscard]] constexpr value_t get_at(int env_depth, int env_index) const {
        return get_impl(ancestor(env_depth)->values_[env_index]);
    }

    constexpr void define(std::string_view name, const value_t& value) {
        const auto it = std::ranges::find(values_, name, &variable_t::name_);
        if (it != values_.end()) {
            assign_impl(*it, value);
        } else {
            if (defining_upvalue()) {
                values_.emplace_back(std::string(name), heap_->create(value));
                defined_upvalue();
            } else {
                values_.emplace_back(std::string(name), value);
            }
        }
    }

    constexpr void assign(const token_t& name, const value_t& value) {
        // non-capturing lambda to avoid unintentionally accessing this
        auto do_assign = [](environment* env, const token_t& name, const value_t& value) {
            for (; env != nullptr; env = env->enclosing_) {
                auto it = std::ranges::find(env->values_, name.lexeme_, &variable_t::name_);
                if (it != env->values_.end()) {
                    assign_impl(*it, value);
                    return;
                }
            }

            throw runtime_error(name, "Undefined variable.");
        };

        do_assign(this, name, value);
    }

    constexpr void assign_at(int env_depth, int env_index, const value_t& value) {
        assign_impl(ancestor(env_depth)->values_[env_index], value);
    }

    template <int arity>
    constexpr void define_native(std::string_view name, auto&& fn) {
        assert(enclosing_ == nullptr && "native functions can only be defined at the global scope");
        using Fn = decltype(fn);
        using NativeFn = native_function<arity, std::decay_t<Fn>>;
        define(name, function(NativeFn(name, std::forward<Fn>(fn))));
    }

private:
    struct variable_t {
        using value = std::variant<value_t, shared_value_t>;

        std::string name_;
        value value_;
    };

    static constexpr value_t get_impl(const variable_t& variable) {
        if (const value_t* v = std::get_if<value_t>(&variable.value_)) {
            return *v;
        } else if (const shared_value_t* uv = std::get_if<shared_value_t>(&variable.value_)) {
            return uv->get();
        } else {
            std::abort();
        }
    }

    static constexpr void assign_impl(variable_t& variable, const value_t& value) {
        if (value_t* v = std::get_if<value_t>(&variable.value_)) {
            *v = value;
        } else if (shared_value_t* uv = std::get_if<shared_value_t>(&variable.value_)) {
            uv->assign(value);
        } else {
            std::abort();
        }
    }

    constexpr bool defining_upvalue() const {
        if (upvalues_.empty())
            return false;

        return upvalues_[0].env_index_ == values_.size();
    }

    constexpr void defined_upvalue() {
        upvalues_ = upvalues_.subspan(1);

        if (upvalues_.empty()) {
            heap_ = nullptr;
        }
    }

    environment* enclosing_ = nullptr;

    heap_t* heap_ = nullptr;
    std::span<const var_index_t> upvalues_;

    // NOTE: vector is only necessary for globals.
    // Other scopes could potentially use static-sized arrays, thanks to the resolver.
    std::vector<variable_t> values_;
};

}  // namespace ctlox::v2