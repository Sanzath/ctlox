#pragma once

#include <ctlox/v2/expression.hpp>
#include <ctlox/v2/statement.hpp>
#include <ctlox/v2/token.hpp>

#include <array>
#include <vector>

namespace ctlox::v2 {

template <typename Statements, typename Expressions, typename Tokens>
struct basic_flat_ast {
    constexpr const flat_stmt_t& operator[](flat_stmt_ptr ptr) const noexcept { return statements_[ptr.i]; }
    constexpr const flat_expr_t& operator[](flat_expr_ptr ptr) const noexcept { return expressions_[ptr.i]; }
    constexpr const token_t& operator[](flat_token_ptr ptr) const noexcept { return tokens_[ptr.i]; }

    constexpr std::span<const token_t> range(flat_token_list list) const noexcept {
        return std::span(tokens_).subspan(list.first_.i, list.size());
    }

    Statements statements_;
    Expressions expressions_;
    Tokens tokens_;

    flat_stmt_list root_block_;
};

using flat_ast = basic_flat_ast<std::vector<flat_stmt_t>, std::vector<flat_expr_t>, std::vector<token_t>>;

template <std::size_t M, std::size_t N, std::size_t P>
struct static_ast : basic_flat_ast<std::array<flat_stmt_t, M>, std::array<flat_expr_t, N>, std::array<token_t, P>> { };

template <typename Ast>
concept _flat_ast = requires(
    const Ast& ast,
    flat_stmt_ptr stmt_ptr,
    flat_expr_ptr expr_ptr,
    flat_token_ptr token_ptr,
    flat_token_list token_list) {
    { ast[stmt_ptr] } -> std::same_as<const flat_stmt_t&>;
    { ast[expr_ptr] } -> std::same_as<const flat_expr_t&>;
    { ast[token_ptr] } -> std::same_as<const token_t&>;
    { ast.range(token_list) } -> std::convertible_to<std::span<const token_t>>;
    { ast.root_block_ } -> std::same_as<const flat_stmt_list&>;
};

}  // namespace ctlox::v2