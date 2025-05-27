#pragma once

#include <ctlox/v2/expression.hpp>
#include <ctlox/v2/statement.hpp>

#include <array>
#include <cassert>
#include <ranges>
#include <span>
#include <vector>

namespace ctlox::v2 {

template <typename Statements, typename Expressions>
struct basic_flat_program {
    constexpr std::span<const flat_stmt_t> root_range() const noexcept { return range(root_block_); }

    constexpr std::span<const flat_stmt_t> range(flat_stmt_list list) const noexcept {
        return std::span(statements_).subspan(list.first_.i, list.size());
    }

    constexpr const flat_stmt_t& operator[](flat_stmt_ptr ptr) const noexcept { return statements_[ptr.i]; }
    constexpr const flat_expr_t& operator[](flat_expr_ptr ptr) const noexcept { return expressions_[ptr.i]; }

    Statements statements_;
    Expressions expressions_;

    flat_stmt_list root_block_;
};

using flat_program = basic_flat_program<std::vector<flat_stmt_t>, std::vector<flat_expr_t>>;

template <std::size_t M, std::size_t N>
struct static_program : basic_flat_program<std::array<flat_stmt_t, M>, std::array<flat_expr_t, N>> { };

class serializer {
public:
    constexpr serializer(std::span<const stmt_ptr> input)
        : input_(input) { }

    constexpr flat_program serialize() && {
        flat_stmt_list root_block = reserve_block(input_.size());

        for (auto [ptr, statement] : std::views::zip(root_block, input_)) {
            put_stmt(ptr, statement->visit(*this));
        }

        return flat_program {
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

        // Use flat_expression_stmt (implicit `.expression_ = flat_nullptr`) as a placeholder.
        statements_.insert(statements_.end(), count, flat_expression_stmt {});

        return block;
    }

    constexpr void put_stmt(flat_stmt_ptr ptr, flat_stmt_t&& stmt) { statements_[ptr.i] = std::move(stmt); }

    constexpr flat_expr_ptr reserve_expr() {
        flat_expr_ptr ptr(expressions_.size());

        // Use flat_literal_expr (implicit `.literal_ = none`) as a placeholder.
        expressions_.push_back(flat_literal_expr {});

        return ptr;
    }

    constexpr void put_expr(flat_expr_ptr ptr, flat_expr_t&& expr) { expressions_[ptr.i] = std::move(expr); }

    std::span<const stmt_ptr> input_;

    std::vector<flat_stmt_t> statements_;
    std::vector<flat_expr_t> expressions_;
};

constexpr flat_program serialize(std::span<const stmt_ptr> input) { return serializer(input).serialize(); }

template <typename Gen>
concept program_generator = requires(Gen gen) {
    { gen() } -> std::convertible_to<std::span<const stmt_ptr>>;
};

template <program_generator auto gen>
constexpr auto static_serialize() {
    constexpr std::pair<std::size_t, std::size_t> sizes = [] {
        const auto program = serialize(gen());
        return std::pair { program.statements_.size(), program.expressions_.size() };
    }();

    constexpr std::size_t M = sizes.first;
    constexpr std::size_t N = sizes.second;

    return []() -> static_program<M, N> {
        const auto flat_program = serialize(gen());

        static_program<M, N> static_program;
        std::ranges::copy(flat_program.statements_, static_program.statements_.begin());
        std::ranges::copy(flat_program.expressions_, static_program.expressions_.begin());
        static_program.root_block_ = flat_program.root_block_;

        return static_program;
    }();
}

}  // namespace ctlox::v2