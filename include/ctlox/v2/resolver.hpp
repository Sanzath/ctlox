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

// TODO: also track amount of references to an env from closures,
//       to know if an env can safely be released after exiting its scope.
//       produce a vector<flat_stmt_ptr> (sorted) of scopes which are closures...
//       or always hold on to every environment,
//       or implement reference counting...

struct local_t {
    flat_expr_ptr ptr_ = flat_nullptr;
    int env_depth_ = -1;
    int env_index_ = -1;

    constexpr std::weak_ordering operator<=>(const local_t& other) const noexcept { return ptr_.i <=> other.ptr_.i; }
    constexpr std::weak_ordering operator<=>(flat_expr_ptr ptr) const noexcept { return ptr_.i <=> ptr.i; }

    constexpr explicit operator bool() const noexcept { return ptr_ != flat_nullptr; }
};

template <typename Locals>
concept _locals = requires(const Locals& locals, std::size_t index) {
    { locals[index] } -> std::convertible_to<local_t>;
};

constexpr local_t find_local(_locals auto const& locals, flat_expr_ptr ptr) noexcept {
    if (auto [it1, it2]
        = std::ranges::equal_range(locals, ptr.i, std::less {}, [](const local_t& local) { return local.ptr_.i; });
        it1 != it2) {
        return *it1;
    }
    return local_t {};
}

template <const auto& ast>
    requires _flat_ast<decltype(ast)>
class resolver {
public:
    constexpr resolver() = default;

    constexpr std::vector<local_t> resolve() && {
        resolve(ast.root_block_);

        std::ranges::sort(locals_, std::less {});
        return std::move(locals_);
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

    constexpr void operator()(flat_stmt_ptr, const flat_block_stmt& stmt) {
        begin_scope();
        resolve(stmt.statements_);
        end_scope();
    }

    constexpr void operator()(flat_stmt_ptr, const flat_break_stmt& stmt) { }

    constexpr void operator()(flat_stmt_ptr, const flat_expression_stmt& stmt) { resolve(stmt.expression_); }

    // constexpr void operator()(const flat_function_stmt& stmt) {}

    constexpr void operator()(flat_stmt_ptr, const flat_if_stmt& stmt) {
        resolve(stmt.condition_);
        resolve(stmt.then_branch_);
        if (stmt.else_branch_ != flat_nullptr)
            resolve(stmt.else_branch_);
    }

    // constexpr void operator()(const flat_return_stmt& stmt) {}

    constexpr void operator()(flat_stmt_ptr, const flat_var_stmt& stmt) {
        declare(stmt.name_);
        if (stmt.initializer_ != flat_nullptr)
            resolve(stmt.initializer_);
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
        if (!scopes_.empty()) {
            auto iter = std::ranges::find(scopes_.back(), expr.name_.lexeme_, &scope_entry_t::name_);
            if (iter != scopes_.back().end() && !iter->defined_) {
                throw parse_error(expr.name_, "Can't read local variable in its own initializer.");
            }
        }

        resolve_local(ptr, expr.name_);
    }

    constexpr void begin_scope() { scopes_.emplace_back(); }

    constexpr void end_scope() { scopes_.pop_back(); }

    constexpr void declare(const token_t& name) {
        if (scopes_.empty())
            return;

        auto& current_scope = scopes_.back();
        if (std::ranges::contains(current_scope, name.lexeme_, &scope_entry_t::name_)) {
            throw parse_error(name, "Already a variable with this name in this scope.");
        }

        current_scope.emplace_back(name.lexeme_, false);
    }

    constexpr void define(const token_t& name) {
        if (scopes_.empty())
            return;

        auto& current_scope = scopes_.back();
        auto iter = std::ranges::find(current_scope, name.lexeme_, &scope_entry_t::name_);
        assert(iter != current_scope.end());
        iter->defined_ = true;
    }

    constexpr void resolve_local(flat_expr_ptr ptr, const token_t& name) {
        for (const auto& [depth, scope] : scopes_ | std::views::reverse | std::views::enumerate) {
            if (auto iter = std::ranges::find(scope, name.lexeme_, &scope_entry_t::name_); iter != scope.end()) {
                const auto index = std::distance(scope.begin(), iter);
                locals_.emplace_back(ptr, depth, index);
                return;
            }
        }
    }

    struct scope_entry_t {
        std::string_view name_;
        bool defined_ = false;
    };

    using scope_t = std::vector<scope_entry_t>;

    std::vector<scope_t> scopes_;
    std::vector<local_t> locals_;
};

template <const auto& ast>
constexpr std::vector<local_t> resolve() {
    return resolver<ast>().resolve();
}

template <const auto& ast>
constexpr auto static_resolve() {
    constexpr std::size_t N = resolve<ast>().size();

    std::array<local_t, N> locals;
    std::ranges::copy(resolve<ast>(), locals.begin());
    return locals;
}

}  // namespace ctlox::v2