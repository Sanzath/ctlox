#pragma once

#include <ctlox/v2/exception.hpp>
#include <ctlox/v2/expression.hpp>
#include <ctlox/v2/flat_ast.hpp>

#include <algorithm>
#include <cassert>
#include <functional>
#include <ranges>
#include <vector>

namespace ctlox::v2 {

// structural span, usable as template parameters/arguments.
template <typename T>
struct span_t {
    const T* data_;
    std::size_t size_;

    constexpr span_t(std::span<const T> span)
        : data_(span.data())
        , size_(span.size()) { }

    constexpr operator std::span<const T>() const noexcept { return std::span(data_, size_); }
};

struct var_index_t {
    int env_depth_ = -1;
    int env_index_ = -1;

    constexpr auto operator<=>(const var_index_t&) const noexcept = default;

    constexpr explicit operator bool() const noexcept { return env_depth_ >= 0; }
};

struct local_t {
    flat_expr_ptr ptr_ = flat_nullptr;
    var_index_t index_;
};

struct scope_t {
    flat_stmt_ptr ptr_ = flat_nullptr;
    flat_list<var_index_t> upvalues_;
};

struct closure_t {
    flat_stmt_ptr ptr_ = flat_nullptr;
    flat_list<var_index_t> upvalues_;
};

template <typename Locals, typename Scopes, typename Closures, typename Upvalues>
struct basic_bindings_t {
    using bindings_tag = void;

    Locals locals_;
    Scopes scopes_;
    Closures closures_;
    Upvalues upvalues_;

    constexpr var_index_t find_local(flat_expr_ptr ptr) const noexcept {
        if (auto [it1, it2] = std::ranges::equal_range(locals_, ptr, {}, &local_t::ptr_); it1 != it2) {
            return it1->index_;
        }
        return {};
    }

    constexpr std::span<const var_index_t> find_scope_upvalues(flat_stmt_ptr ptr) const noexcept {
        if (auto [it1, it2] = std::ranges::equal_range(scopes_, ptr, {}, &scope_t::ptr_); it1 != it2) {
            const flat_list<var_index_t>& list = it1->upvalues_;
            return std::span(upvalues_).subspan(list.first_.i, list.size());
        }
        return {};
    }

    constexpr std::span<const var_index_t> find_closure_upvalues(flat_stmt_ptr ptr) const noexcept {
        if (auto [it1, it2] = std::ranges::equal_range(closures_, ptr, {}, &closure_t::ptr_); it1 != it2) {
            const flat_list<var_index_t>& list = it1->upvalues_;
            return std::span(upvalues_).subspan(list.first_.i, list.size());
        }
        return {};
    }
};

template <typename B>
concept _bindings = requires { typename std::remove_reference_t<B>::bindings_tag; };

using bindings_t
    = basic_bindings_t<std::vector<local_t>, std::vector<scope_t>, std::vector<closure_t>, std::vector<var_index_t>>;

template <std::size_t L, std::size_t S, std::size_t C, std::size_t U>
using static_bindings_t = basic_bindings_t<
    std::array<local_t, L>,
    std::array<scope_t, S>,
    std::array<closure_t, C>,
    std::array<var_index_t, U>>;

class _resolver_base {
protected:
    struct scope_entry_t {
        std::string_view name_;
        bool defined_ = false;
        bool captured_ = false;
    };

    struct context {
        enum class type {
            scope,
            function,
        };

        constexpr context(flat_stmt_ptr ptr, type type)
            : ptr_(ptr)
            , type_(type) { }

        flat_stmt_ptr ptr_ = flat_nullptr;
        type type_;

        context* enclosing_ = nullptr;

        std::vector<scope_entry_t> scope_entries_;
        std::vector<var_index_t> upvalue_entries_;

        constexpr scope_entry_t* find_entry(std::string_view name) noexcept {
            auto it = std::ranges::find(scope_entries_, name, &scope_entry_t::name_);
            return it != std::ranges::end(scope_entries_) ? &*it : nullptr;
        }

        constexpr void declare_entry(std::string_view name) noexcept {
            scope_entries_.emplace_back(name, false, false);
        }

        constexpr void define_entry(std::string_view name) noexcept {
            scope_entry_t* entry = find_entry(name);
            assert(entry != nullptr);
            entry->defined_ = true;
        }

        // example resolve:
        // (1.block) a, b / (2.block) x, y / (3.function) c, d / (4.block) e
        // resolve "b" from innermost scope:
        // - 3 scope boundaries from 4.block to 1.block
        // - variable at index 1 of 1.block
        // > var_index_t { env_depth_ = 3, env_index_ = 1 }
        // function "outer" closure index:
        // - 2 scope boundaries from 3.function to 1.block
        // - same variable index = 1
        // > var_index_t { env_depth_ = 2, env_index_ = 1 }
        // function "inner" local index:
        // - 1 scope boundary from 4 to 3, + 1 because the closure is actually
        //   the function scope's enclosing environment, depth = 2
        // - this is the first captured variable in 3.function, so index = 0
        // > var_index_t { env_depth_ = 2, env_index_ = 0 }
        constexpr var_index_t resolve_local(std::string_view name, bool capture = false) noexcept {
            scope_entry_t* entry = find_entry(name);
            if (entry) {
                if (capture) {
                    entry->captured_ = true;
                }

                int index = static_cast<int>(std::distance(scope_entries_.data(), entry));
                return { 0, index };
            }

            if (!enclosing_) {
                return {};
            }

            if (type_ != type::function) {
                auto local = enclosing_->resolve_local(name, capture);
                if (local)
                    ++local.env_depth_;
                return local;
            } else {
                auto upvalue = enclosing_->resolve_local(name, true);
                if (upvalue) {
                    int upvalue_index = add_upvalue(upvalue);
                    // Closure upvalues actually live 1 step above the function's root scope.
                    return { 1, upvalue_index };
                }
                return {};
            }
        }

        constexpr int add_upvalue(var_index_t upvalue) noexcept {
            if (auto it = std::ranges::find(upvalue_entries_, upvalue); it != upvalue_entries_.end()) {
                return std::distance(upvalue_entries_.begin(), it);
            } else {
                upvalue_entries_.emplace_back(upvalue);
                return upvalue_entries_.size() - 1;
            }
        }

        constexpr std::vector<var_index_t> get_scope_upvalues() const noexcept {
            std::vector<var_index_t> upvalues;
            upvalues.reserve(scope_entries_.size());
            for (const auto& [index, entry] : std::views::enumerate(scope_entries_)) {
                if (entry.captured_) {
                    upvalues.emplace_back(0, index);
                }
            }
            return upvalues;
        }
    };
};

template <const auto& ast>
    requires _flat_ast<decltype(ast)>
class resolver : _resolver_base {
public:
    constexpr resolver() = default;

    constexpr bindings_t resolve() && {
        resolve(ast.root_block_);

        std::ranges::sort(locals_, {}, &local_t::ptr_);
        std::ranges::sort(scopes_, {}, &scope_t::ptr_);
        std::ranges::sort(closures_, {}, &closure_t::ptr_);

        return bindings_t {
            .locals_ = std::move(locals_),
            .scopes_ = std::move(scopes_),
            .closures_ = std::move(closures_),
            .upvalues_ = std::move(upvalues_),
        };
    }

private:
    constexpr void resolve(flat_stmt_ptr ptr) {
        ast[ptr].visit([this, ptr](const auto& stmt) { (*this)(ptr, stmt); });
    }
    constexpr void resolve(flat_expr_ptr ptr) {
        ast[ptr].visit([this, ptr](const auto& expr) { (*this)(ptr, expr); });
    }

    constexpr void resolve(flat_stmt_list stmts) {
        for (flat_stmt_ptr stmt : stmts) {
            resolve(stmt);
        }
    }

    constexpr void resolve(flat_expr_list exprs) {
        for (flat_expr_ptr expr : exprs) {
            resolve(expr);
        }
    }

    constexpr void operator()(flat_stmt_ptr ptr, const flat_block_stmt& stmt) {
        context ctx(ptr, context::type::scope);

        begin_ctx(ctx);
        resolve(stmt.statements_);
        end_ctx(ctx);
    }

    constexpr void operator()(flat_stmt_ptr, const flat_break_stmt& stmt) { }

    constexpr void operator()(flat_stmt_ptr, const flat_expression_stmt& stmt) { resolve(stmt.expression_); }

    constexpr void operator()(flat_stmt_ptr ptr, const flat_function_stmt& stmt) {
        declare(stmt.name_);
        define(stmt.name_);

        resolve_function(ptr, stmt);
    }

    constexpr void operator()(flat_stmt_ptr, const flat_if_stmt& stmt) {
        resolve(stmt.condition_);
        resolve(stmt.then_branch_);
        if (stmt.else_branch_ != flat_nullptr) {
            resolve(stmt.else_branch_);
        }
    }

    constexpr void operator()(flat_stmt_ptr, const flat_return_stmt& stmt) {
        if (stmt.value_ != flat_nullptr) {
            resolve(stmt.value_);
        }
    }

    constexpr void operator()(flat_stmt_ptr, const flat_var_stmt& stmt) {
        declare(stmt.name_);
        if (stmt.initializer_ != flat_nullptr) {
            resolve(stmt.initializer_);
        }
        define(stmt.name_);
    }

    constexpr void operator()(flat_stmt_ptr, const flat_while_stmt& stmt) {
        resolve(stmt.condition_);
        resolve(stmt.body_);
    }

    constexpr void operator()(flat_expr_ptr ptr, const flat_assign_expr& expr) {
        resolve(expr.value_);
        resolve_local(ptr, expr.name_);
    }

    constexpr void operator()(flat_expr_ptr, const flat_binary_expr& expr) {
        resolve(expr.left_);
        resolve(expr.right_);
    }

    constexpr void operator()(flat_expr_ptr, const flat_call_expr& expr) {
        resolve(expr.callee_);
        resolve(expr.arguments_);
    }

    constexpr void operator()(flat_expr_ptr, const flat_grouping_expr& expr) { resolve(expr.expr_); }

    constexpr void operator()(flat_expr_ptr, const flat_literal_expr& expr) { }

    constexpr void operator()(flat_expr_ptr, const flat_logical_expr& expr) {
        resolve(expr.left_);
        resolve(expr.right_);
    }

    constexpr void operator()(flat_expr_ptr, const flat_unary_expr& expr) { resolve(expr.right_); }

    constexpr void operator()(flat_expr_ptr ptr, const flat_variable_expr& expr) {
        if (ctx_) {
            if (scope_entry_t* entry = ctx_->find_entry(expr.name_.lexeme_); entry && !entry->defined_) {
                throw parse_error(expr.name_, "Can't read local variable in its own initializer.");
            }
        }

        resolve_local(ptr, expr.name_);
    }

    constexpr void begin_ctx(context& ctx) {
        ctx.enclosing_ = std::exchange(ctx_, &ctx);
    }

    constexpr void end_ctx(context& ctx) {
        assert(ctx_ == &ctx);
        ctx_ = std::exchange(ctx.enclosing_, nullptr);

        if (ctx.type_ == context::type::function) {
            if (!ctx.upvalue_entries_.empty()) {
                flat_list<var_index_t> list = append_upvalues(ctx.upvalue_entries_);
                closures_.emplace_back(ctx.ptr_, list);
            }
        }

        auto upvalues = ctx.get_scope_upvalues();
        if (!upvalues.empty()) {
            flat_list<var_index_t> list = append_upvalues(upvalues);
            scopes_.emplace_back(ctx.ptr_, list);
        }
    }

    constexpr void declare(const token_t& name) {
        if (!ctx_)
            return;

        if (ctx_->find_entry(name.lexeme_)) {
            throw parse_error(name, "Already a variable with this name in this scope.");
        }

        ctx_->declare_entry(name.lexeme_);
    }

    constexpr void define(const token_t& name) {
        if (!ctx_)
            return;

        ctx_->define_entry(name.lexeme_);
    }

    constexpr void resolve_function(flat_stmt_ptr ptr, const flat_function_stmt& function) {
        context ctx(ptr, context::type::function);
        begin_ctx(ctx);
        for (const token_t& param : ast.range(function.params_)) {
            declare(param);
            define(param);
        }
        resolve(function.body_);
        end_ctx(ctx);
    }

    constexpr void resolve_local(flat_expr_ptr ptr, const token_t& name) {
        if (!ctx_)
            return;

        auto var_index = ctx_->resolve_local(name.lexeme_);
        if (var_index) {
            locals_.emplace_back(ptr, var_index);
        }
    }

    constexpr flat_list<var_index_t> append_upvalues(const std::vector<var_index_t>& upvalues) {
        flat_list<var_index_t> list = {
            .first_ = { upvalues_.size() },
            .last_ = { upvalues_.size() + upvalues.size() },
        };
        upvalues_.append_range(upvalues);
        return list;
    }

    std::vector<local_t> locals_;
    std::vector<scope_t> scopes_;
    std::vector<closure_t> closures_;
    std::vector<var_index_t> upvalues_;

    context* ctx_ = nullptr;
};

template <const auto& ast>
constexpr bindings_t resolve() {
    return resolver<ast>().resolve();
}

template <const auto& ast>
constexpr _bindings auto static_resolve() {
    constexpr std::array<std::size_t, 4> sizes = [] {
        bindings_t bindings = resolve<ast>();
        return std::array {
            bindings.locals_.size(),
            bindings.scopes_.size(),
            bindings.closures_.size(),
            bindings.upvalues_.size(),
        };
    }();

    constexpr std::size_t L = sizes[0];
    constexpr std::size_t S = sizes[1];
    constexpr std::size_t C = sizes[2];
    constexpr std::size_t U = sizes[3];

    return [] {
        bindings_t bindings = resolve<ast>();

        static_bindings_t<L, S, C, U> static_bindings;
        std::ranges::copy(bindings.locals_, static_bindings.locals_.begin());
        std::ranges::copy(bindings.scopes_, static_bindings.scopes_.begin());
        std::ranges::copy(bindings.closures_, static_bindings.closures_.begin());
        std::ranges::copy(bindings.upvalues_, static_bindings.upvalues_.begin());
        return static_bindings;
    }();
}

}  // namespace ctlox::v2