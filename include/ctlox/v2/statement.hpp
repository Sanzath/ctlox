#pragma once

#include <ctlox/v2/expression.hpp>
#include <ctlox/v2/flat.hpp>
#include <ctlox/v2/token.hpp>

#include <memory>
#include <variant>
#include <vector>

namespace ctlox::v2 {

class stmt_t;
using stmt_ptr = std::unique_ptr<stmt_t>;
using stmt_list = std::vector<stmt_ptr>;

class flat_stmt_t;
using flat_stmt_ptr = flat_ptr<flat_stmt_t>;
using flat_stmt_list = flat_list<flat_stmt_t>;

template <typename StmtList>
struct basic_block_stmt {
    StmtList statements_;
};

using block_stmt = basic_block_stmt<stmt_list>;
using flat_block_stmt = basic_block_stmt<flat_stmt_list>;

template <typename ExprPtr>
struct basic_expression_stmt {
    ExprPtr expression_;
};

using expression_stmt = basic_expression_stmt<expr_ptr>;
using flat_expression_stmt = basic_expression_stmt<flat_expr_ptr>;

template <typename ExprPtr>
struct basic_print_stmt {
    ExprPtr expression_;
};

using print_stmt = basic_print_stmt<expr_ptr>;
using flat_print_stmt = basic_print_stmt<flat_expr_ptr>;

template <typename ExprPtr>
struct basic_var_stmt {
    token_t name_;
    ExprPtr initializer_;
};

using var_stmt = basic_var_stmt<expr_ptr>;
using flat_var_stmt = basic_var_stmt<flat_expr_ptr>;

template <typename StmtList, typename ExprPtr>
class basic_stmt_t {
    using variant_t = std::variant<
        basic_block_stmt<StmtList>,
        basic_expression_stmt<ExprPtr>,
        basic_print_stmt<ExprPtr>,
        basic_var_stmt<ExprPtr>>;

    variant_t stmt_;

public:
    template <typename Stmt>
        requires std::constructible_from<variant_t, Stmt&&>
    constexpr basic_stmt_t(Stmt&& stmt)
        : stmt_(std::forward<Stmt>(stmt)) { }

    constexpr ~basic_stmt_t() noexcept = default;

    template <typename Visitor>
    constexpr auto visit(Visitor&& v) const {
        return std::visit(std::forward<Visitor>(v), stmt_);
    }

    template <typename Stmt>
    [[nodiscard]] constexpr bool holds() const noexcept {
        return std::holds_alternative<Stmt>(stmt_);
    }

    template <typename Stmt>
    [[nodiscard]] constexpr const Stmt* get_if() const noexcept {
        return std::get_if<Stmt>(&stmt_);
    }
};

// These types need to be defined as a classes rather than aliases so that
// they can refer to themselves through their pointer types.
class stmt_t : public basic_stmt_t<stmt_list, expr_ptr> {
public:
    using basic_stmt_t::basic_stmt_t;
    constexpr ~stmt_t() noexcept = default;
};
class flat_stmt_t : public basic_stmt_t<flat_stmt_list, flat_expr_ptr> {
public:
    using basic_stmt_t::basic_stmt_t;
    constexpr ~flat_stmt_t() noexcept = default;
};

template <typename Stmt>
constexpr stmt_ptr make_stmt(Stmt&& stmt) {
    return std::make_unique<stmt_t>(std::forward<Stmt>(stmt));
}

}  // namespace ctlox::v2