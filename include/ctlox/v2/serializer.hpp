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
        flat_stmt_list root_block = reserve_stmt_list(input_.size());

        for (auto [ptr, statement] : std::views::zip(root_block, input_)) {
            put_stmt(ptr, statement->visit(*this));
        }

        return flat_ast {
            .statements_ = std::move(statements_),
            .expressions_ = std::move(expressions_),
            .tokens_ = std::move(tokens_),
            .root_block_ = root_block,
        };
    }

    constexpr flat_stmt_t operator()(const break_stmt& statement) { return statement; }

    constexpr flat_stmt_t operator()(const block_stmt& statement) {
        auto statements = std::span(statement.statements_);
        flat_stmt_list block = reserve_stmt_list(statements.size());

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

    constexpr flat_stmt_t operator()(const function_stmt& statement) {
        flat_token_list params = put_tokens(statement.params_);
        flat_stmt_list body = reserve_stmt_list(statement.body_.size());
        for (auto [ptr, statement] : std::views::zip(body, statement.body_)) {
            put_stmt(ptr, statement->visit(*this));
        }

        return flat_function_stmt {
            .name_ = statement.name_,
            .params_ = params,
            .body_ = body,
        };
    }

    constexpr flat_stmt_t operator()(const if_stmt& statement) {
        flat_expr_ptr condition = reserve_expr();
        flat_stmt_ptr then_branch = reserve_stmt();
        flat_stmt_ptr else_branch;
        if (statement.else_branch_) {
            else_branch = reserve_stmt();
        }

        put_expr(condition, statement.condition_->visit(*this));
        put_stmt(then_branch, statement.then_branch_->visit(*this));
        if (statement.else_branch_) {
            put_stmt(else_branch, statement.else_branch_->visit(*this));
        }

        return flat_if_stmt {
            .condition_ = condition,
            .then_branch_ = then_branch,
            .else_branch_ = else_branch,
        };
    }

    constexpr flat_stmt_t operator()(const return_stmt& statement) {
        flat_expr_ptr value;
        if (statement.value_) {
            value = reserve_expr();
            put_expr(value, statement.value_->visit(*this));
        }

        return flat_return_stmt { .keyword_ = statement.keyword_, .value_ = value };
    }

    constexpr flat_stmt_t operator()(const var_stmt& statement) {
        flat_expr_ptr ptr;
        if (statement.initializer_) {
            ptr = reserve_expr();
            put_expr(ptr, statement.initializer_->visit(*this));
        }

        return flat_var_stmt { .name_ = statement.name_, .initializer_ = ptr };
    }

    constexpr flat_stmt_t operator()(const while_stmt& statement) {
        flat_expr_ptr condition = reserve_expr();
        flat_stmt_ptr body = reserve_stmt();

        put_expr(condition, statement.condition_->visit(*this));
        put_stmt(body, statement.body_->visit(*this));

        return flat_while_stmt { .condition_ = condition, .body_ = body };
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

    constexpr flat_expr_t operator()(const call_expr& expression) {
        flat_expr_ptr callee = reserve_expr();
        flat_expr_list arguments = reserve_expr_list(expression.arguments_.size());

        put_expr(callee, expression.callee_->visit(*this));
        for (auto [ptr, argument] : std::views::zip(arguments, expression.arguments_)) {
            put_expr(ptr, argument->visit(*this));
        }

        return flat_call_expr {
            .paren_ = expression.paren_,
            .callee_ = callee,
            .arguments_ = arguments,
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

    constexpr flat_expr_t operator()(const logical_expr& expression) {
        flat_expr_ptr left = reserve_expr();
        flat_expr_ptr right = reserve_expr();

        put_expr(left, expression.left_->visit(*this));
        put_expr(right, expression.right_->visit(*this));

        return flat_logical_expr {
            .operator_ = expression.operator_,
            .left_ = left,
            .right_ = right,
        };
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
    constexpr flat_stmt_list reserve_stmt_list(std::size_t count) {
        flat_stmt_list stmt_list {
            .first_ { statements_.size() },
            .last_ { statements_.size() + count },
        };

        statements_.resize(statements_.size() + count);

        return stmt_list;
    }

    constexpr flat_stmt_ptr reserve_stmt() {
        flat_stmt_ptr ptr(statements_.size());

        statements_.emplace_back();

        return ptr;
    }

    constexpr void put_stmt(flat_stmt_ptr ptr, flat_stmt_t&& stmt) { statements_[ptr.i] = std::move(stmt); }

    constexpr flat_expr_list reserve_expr_list(std::size_t count) {
        flat_expr_list expr_list {
            .first_ = { expressions_.size() },
            .last_ = { expressions_.size() + count },
        };

        expressions_.resize(expressions_.size() + count);

        return expr_list;
    }

    constexpr flat_expr_ptr reserve_expr() {
        flat_expr_ptr ptr(expressions_.size());

        expressions_.emplace_back();

        return ptr;
    }

    constexpr void put_expr(flat_expr_ptr ptr, flat_expr_t&& expr) { expressions_[ptr.i] = std::move(expr); }

    constexpr flat_token_list put_tokens(std::span<const token_t> tokens) {
        flat_token_list token_list {
            .first_ = { tokens_.size() },
            .last_ = { tokens_.size() + tokens.size() },
        };

        tokens_.append_range(tokens);

        return token_list;
    }

    std::span<const stmt_ptr> input_;

    std::vector<flat_stmt_t> statements_;
    std::vector<flat_expr_t> expressions_;
    std::vector<token_t> tokens_;
};

constexpr flat_ast serialize(std::span<const stmt_ptr> input) { return serializer(input).serialize(); }

template <typename Gen>
concept ast_generator = requires(Gen gen) {
    { gen() } -> std::convertible_to<std::span<const stmt_ptr>>;
};

template <ast_generator auto gen>
constexpr auto static_serialize() {
    constexpr std::array<std::size_t, 3> sizes = [] {
        const auto ast = serialize(gen());
        return std::array{ ast.statements_.size(), ast.expressions_.size(), ast.tokens_.size() };
    }();

    constexpr std::size_t M = sizes[0];
    constexpr std::size_t N = sizes[1];
    constexpr std::size_t P = sizes[2];

    return []() -> static_ast<M, N, P> {
        const auto flat_ast = serialize(gen());

        static_ast<M, N, P> static_ast;
        std::ranges::copy(flat_ast.statements_, static_ast.statements_.begin());
        std::ranges::copy(flat_ast.expressions_, static_ast.expressions_.begin());
        std::ranges::copy(flat_ast.tokens_, static_ast.tokens_.begin());
        static_ast.root_block_ = flat_ast.root_block_;

        return static_ast;
    }();
}

}  // namespace ctlox::v2