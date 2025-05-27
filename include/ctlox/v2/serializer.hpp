#pragma once

#include <ctlox/v2/expression.hpp>
#include <ctlox/v2/statement.hpp>

#include <ctlox/v2/flat_ast.hpp>

#include <ranges>
#include <span>
#include <vector>

namespace ctlox::v2 {

class serializer {
public:
    constexpr serializer(std::span<const stmt_ptr> input)
        : input_(input) { }

    constexpr flat_ast serialize() && {
        flat_stmt_list root_block = reserve_block(input_.size());

        for (auto [ptr, statement] : std::views::zip(root_block, input_)) {
            put_stmt(ptr, statement->visit(*this));
        }

        return flat_ast {
            std::move(statements_),
            std::move(expressions_),
            root_block,
        };
    }

    constexpr flat_stmt_t operator()(const block_stmt& statement) {
        auto statements = std::span(statement.statements_);
        flat_stmt_list block = reserve_block(statements.size());

        for (auto [ptr, statement] : std::views::zip(block, statements)) {
            put_stmt(ptr, statement->visit(*this));
        }

        return flat_block_stmt { .statements_ = block };
    }

    constexpr flat_stmt_t operator()(const expression_stmt& statement) {
        flat_expr_ptr ptr = reserve_expr();
        put_expr(ptr, statement.expression_->visit(*this));

        return flat_expression_stmt { .expression_ = ptr };
    }

    constexpr flat_stmt_t operator()(const print_stmt& statement) {
        flat_expr_ptr ptr = reserve_expr();
        put_expr(ptr, statement.expression_->visit(*this));

        return flat_print_stmt { .expression_ = ptr };
    }

    constexpr flat_stmt_t operator()(const var_stmt& statement) {
        flat_expr_ptr ptr;
        if (statement.initializer_) {
            ptr = reserve_expr();
            put_expr(ptr, statement.initializer_->visit(*this));
        }

        return flat_var_stmt { .name_ = statement.name_, .initializer_ = ptr };
    }

    constexpr flat_expr_t operator()(const assign_expr& expression) {
        flat_expr_ptr ptr = reserve_expr();
        put_expr(ptr, expression.value_->visit(*this));

        return flat_assign_expr { .name_ = expression.name_, .value_ = ptr };
    }

    constexpr flat_expr_t operator()(const binary_expr& expression) {
        flat_expr_ptr left = reserve_expr();
        flat_expr_ptr right = reserve_expr();

        put_expr(left, expression.left_->visit(*this));
        put_expr(right, expression.right_->visit(*this));

        return flat_binary_expr {
            .operator_ = expression.operator_,
            .left_ = left,
            .right_ = right,
        };
    }

    constexpr flat_expr_t operator()(const grouping_expr& expression) {
        // NOTE: grouping_expr is technically useful during the parse, but not so much
        // post-parse. The following would also be valid.
        // return expression.expr_->visit(*this);

        flat_expr_ptr ptr = reserve_expr();
        put_expr(ptr, expression.expr_->visit(*this));

        return flat_grouping_expr { .expr_ = ptr };
    }

    constexpr flat_expr_t operator()(const literal_expr& expression) {
        static_assert(std::is_same_v<literal_expr, flat_literal_expr>);
        return expression;
    }

    constexpr flat_expr_t operator()(const unary_expr& expression) {
        flat_expr_ptr ptr = reserve_expr();
        put_expr(ptr, expression.right_->visit(*this));

        return flat_unary_expr { .operator_ = expression.operator_, .right_ = ptr };
    }

    constexpr flat_expr_t operator()(const variable_expr& expression) {
        static_assert(std::is_same_v<variable_expr, flat_variable_expr>);
        return expression;
    }

private:
    constexpr flat_stmt_list reserve_block(std::size_t count) {
        flat_stmt_list block {
            .first_ { statements_.size() },
            .last_ { statements_.size() + count },
        };

        statements_.resize(statements_.size() + count);

        return block;
    }

    constexpr void put_stmt(flat_stmt_ptr ptr, flat_stmt_t&& stmt) { statements_[ptr.i] = std::move(stmt); }

    constexpr flat_expr_ptr reserve_expr() {
        flat_expr_ptr ptr(expressions_.size());

        expressions_.emplace_back();

        return ptr;
    }

    constexpr void put_expr(flat_expr_ptr ptr, flat_expr_t&& expr) { expressions_[ptr.i] = std::move(expr); }

    std::span<const stmt_ptr> input_;

    std::vector<flat_stmt_t> statements_;
    std::vector<flat_expr_t> expressions_;
};

constexpr flat_ast serialize(std::span<const stmt_ptr> input) { return serializer(input).serialize(); }

template <typename Gen>
concept ast_generator = requires(Gen gen) {
    { gen() } -> std::convertible_to<std::span<const stmt_ptr>>;
};

template <ast_generator auto gen>
constexpr auto static_serialize() {
    constexpr std::pair<std::size_t, std::size_t> sizes = [] {
        const auto ast = serialize(gen());
        return std::pair { ast.statements_.size(), ast.expressions_.size() };
    }();

    constexpr std::size_t M = sizes.first;
    constexpr std::size_t N = sizes.second;

    return []() -> static_ast<M, N> {
        const auto flat_ast = serialize(gen());

        static_ast<M, N> static_ast;
        std::ranges::copy(flat_ast.statements_, static_ast.statements_.begin());
        std::ranges::copy(flat_ast.expressions_, static_ast.expressions_.begin());
        static_ast.root_block_ = flat_ast.root_block_;

        return static_ast;
    }();
}

}  // namespace ctlox::v2