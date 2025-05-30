#pragma once

#include <ctlox/v2/expression.hpp>
#include <ctlox/v2/statement.hpp>

#include <array>
#include <span>
#include <vector>

namespace ctlox::v2 {

template <typename Statements, typename Expressions>
struct basic_flat_ast {
    constexpr const flat_stmt_t& operator[](flat_stmt_ptr ptr) const noexcept { return statements_[ptr.i]; }
    constexpr const flat_expr_t& operator[](flat_expr_ptr ptr) const noexcept { return expressions_[ptr.i]; }

    Statements statements_;
    Expressions expressions_;

    flat_stmt_list root_block_;
};

using flat_ast = basic_flat_ast<std::vector<flat_stmt_t>, std::vector<flat_expr_t>>;

template <std::size_t M, std::size_t N>
struct static_ast : basic_flat_ast<std::array<flat_stmt_t, M>, std::array<flat_expr_t, N>> { };

template <typename Ast>
concept flat_ast_c = requires(const Ast& ast, flat_stmt_ptr stmt_ptr, flat_expr_ptr expr_ptr) {
    { ast[stmt_ptr] } -> std::same_as<const flat_stmt_t&>;
    { ast[expr_ptr] } -> std::same_as<const flat_expr_t&>;
};

}  // namespace ctlox::v2