#pragma once

#include <ctlox/v2/expression.hpp>
#include <ctlox/v2/statement.hpp>

#include <cassert>
#include <ranges>
#include <span>
#include <vector>

namespace ctlox::v2 {

struct flat_statements_t {
    std::vector<flat_stmt_t> statements_;
    std::vector<flat_expr_t> expressions_;

    flat_stmt_list_t root_block_;
};

class flattener {
public:
    constexpr flattener(std::span<const stmt_ptr_t> input)
        : input_(input) { }

    constexpr flat_statements_t flatten() && {
        flat_stmt_list_t root_block = reserve_block(input_.size());

        for (auto [ptr, statement] : std::views::zip(root_block, input_)) {
            put_stmt(ptr, statement->visit(*this));
        }

        return flat_statements_t {
            std::move(statements_),
            std::move(expressions_),
            root_block,
        };
    }

    constexpr flat_stmt_t operator()(const block_stmt& statement) {
        auto statements = std::span(statement.statements_);
        flat_stmt_list_t block = reserve_block(statements.size());

        for (auto [ptr, statement] : std::views::zip(block, statements)) {
            put_stmt(ptr, statement->visit(*this));
        }

        return flat_block_stmt { .statements_ = block };
    }

    constexpr flat_stmt_t operator()(const expression_stmt& statement) {
        flat_expr_ptr_t ptr = reserve_expr();
        put_expr(ptr, statement.expression_->visit(*this));

        return flat_expression_stmt { .expression_ = ptr };
    }

    constexpr flat_stmt_t operator()(const print_stmt& statement) {
        flat_expr_ptr_t ptr = reserve_expr();
        put_expr(ptr, statement.expression_->visit(*this));

        return flat_print_stmt { .expression_ = ptr };
    }

    constexpr flat_stmt_t operator()(const var_stmt& statement) {
        flat_expr_ptr_t ptr;
        if (statement.initializer_) {
            ptr = reserve_expr();
            put_expr(ptr, statement.initializer_->visit(*this));
        }

        return flat_var_stmt { .name_ = statement.name_, .initializer_ = ptr };
    }

    constexpr flat_expr_t operator()(const assign_expr& expression) {
        flat_expr_ptr_t ptr = reserve_expr();
        put_expr(ptr, expression.value_->visit(*this));

        return flat_assign_expr { .name_ = expression.name_, .value_ = ptr };
    }

    constexpr flat_expr_t operator()(const binary_expr& expression) {
        flat_expr_ptr_t left = reserve_expr();
        flat_expr_ptr_t right = reserve_expr();

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

        flat_expr_ptr_t ptr = reserve_expr();
        put_expr(ptr, expression.expr_->visit(*this));

        return flat_grouping_expr { .expr_ = ptr };
    }

    constexpr flat_expr_t operator()(const literal_expr& expression) {
        static_assert(std::is_same_v<literal_expr, flat_literal_expr>);
        return expression;
    }

    constexpr flat_expr_t operator()(const unary_expr& expression) {
        flat_expr_ptr_t ptr = reserve_expr();
        put_expr(ptr, expression.right_->visit(*this));

        return flat_unary_expr { .operator_ = expression.operator_, .right_ = ptr };
    }

    constexpr flat_expr_t operator()(const variable_expr& expression) {
        static_assert(std::is_same_v<variable_expr, flat_variable_expr>);
        return expression;
    }

private:
    constexpr flat_stmt_list_t reserve_block(std::size_t count) {
        flat_stmt_list_t block {
            .first_ { statements_.size() },
            .last_ { statements_.size() + count },
        };

        // Use flat_expression_stmt (implicit `.expression_ = flat_nullptr`) as a placeholder.
        statements_.insert(statements_.end(), count, flat_expression_stmt {});

        return block;
    }

    constexpr void put_stmt(flat_stmt_ptr_t ptr, flat_stmt_t&& stmt) { statements_[ptr.i] = std::move(stmt); }

    constexpr flat_expr_ptr_t reserve_expr() {
        flat_expr_ptr_t ptr(expressions_.size());

        // Use flat_literal_expr (implicit `.literal_ = none`) as a placeholder.
        expressions_.push_back(flat_literal_expr {});

        return ptr;
    }

    constexpr void put_expr(flat_expr_ptr_t ptr, flat_expr_t&& expr) { expressions_[ptr.i] = std::move(expr); }

    std::span<const stmt_ptr_t> input_;

    std::vector<flat_stmt_t> statements_;
    std::vector<flat_expr_t> expressions_;
};

constexpr flat_statements_t flatten(std::span<const stmt_ptr_t> input) { return flattener(input).flatten(); }

}  // namespace ctlox::v2