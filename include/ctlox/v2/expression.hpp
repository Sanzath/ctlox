#pragma once

#include <ctlox/v2/flat_ptr.hpp>
#include <ctlox/v2/static_visit.hpp>
#include <ctlox/v2/token.hpp>

#include <memory>
#include <variant>
#include <vector>

namespace ctlox::v2 {

class expr_t;
using expr_ptr = std::unique_ptr<expr_t>;
using expr_list = std::vector<expr_ptr>;

class flat_expr_t;
using flat_expr_ptr = flat_ptr<flat_expr_t>;
using flat_expr_list = flat_list<flat_expr_t>;

template <typename Expr>
struct expr_traits { };

template <>
struct expr_traits<expr_t> {
    using ptr = expr_ptr;
    using list = expr_list;
};

template <>
struct expr_traits<flat_expr_t> {
    using ptr = flat_expr_ptr;
    using list = flat_expr_list;
};

template <typename ExprPtr>
struct basic_assign_expr {
    token_t name_;
    ExprPtr value_;
};

using assign_expr = basic_assign_expr<expr_ptr>;
using flat_assign_expr = basic_assign_expr<flat_expr_ptr>;

template <typename ExprPtr>
struct basic_binary_expr {
    token_t operator_;
    ExprPtr left_;
    ExprPtr right_;
};

using binary_expr = basic_binary_expr<expr_ptr>;
using flat_binary_expr = basic_binary_expr<flat_expr_ptr>;

template <typename ExprPtr, typename ExprList>
struct basic_call_expr {
    token_t paren_;
    ExprPtr callee_;
    ExprList arguments_;
};

using call_expr = basic_call_expr<expr_ptr, expr_list>;
using flat_call_expr = basic_call_expr<flat_expr_ptr, flat_expr_list>;

template <typename ExprPtr>
struct basic_grouping_expr {
    ExprPtr expr_;
};

using grouping_expr = basic_grouping_expr<expr_ptr>;
using flat_grouping_expr = basic_grouping_expr<flat_expr_ptr>;

struct basic_literal_expr {
    literal_t value_;
};

using literal_expr = basic_literal_expr;
using flat_literal_expr = basic_literal_expr;

template <typename ExprPtr>
struct basic_logical_expr {
    token_t operator_;
    ExprPtr left_;
    ExprPtr right_;
};

using logical_expr = basic_logical_expr<expr_ptr>;
using flat_logical_expr = basic_logical_expr<flat_expr_ptr>;

template <typename ExprPtr>
struct basic_unary_expr {
    token_t operator_;
    ExprPtr right_;
};

using unary_expr = basic_unary_expr<expr_ptr>;
using flat_unary_expr = basic_unary_expr<flat_expr_ptr>;

struct basic_variable_expr {
    token_t name_;
};

using variable_expr = basic_variable_expr;
using flat_variable_expr = basic_variable_expr;

template <typename ExprT>
class basic_expr_t {
    using ExprPtr = typename expr_traits<ExprT>::ptr;
    using ExprList = typename expr_traits<ExprT>::list;

    using variant_t = std::variant<
        basic_assign_expr<ExprPtr>,
        basic_binary_expr<ExprPtr>,
        basic_call_expr<ExprPtr, ExprList>,
        basic_grouping_expr<ExprPtr>,
        basic_literal_expr,
        basic_logical_expr<ExprPtr>,
        basic_unary_expr<ExprPtr>,
        basic_variable_expr>;

    variant_t expr_;

public:
    template <typename Expr>
        requires std::constructible_from<variant_t, Expr&&>
    constexpr basic_expr_t(Expr&& expr) noexcept  // NOLINT(*-explicit-constructor)
        : expr_(std::forward<Expr>(expr)) { }

    constexpr basic_expr_t() noexcept
        : expr_(std::in_place_type<basic_literal_expr>) { }

    constexpr ~basic_expr_t() noexcept = default;

    template <typename Visitor>
    constexpr auto visit(Visitor&& v) const {
        return std::visit(std::forward<Visitor>(v), expr_);
    }

    template <typename Expr>
    [[nodiscard]] constexpr bool holds() const noexcept {
        return std::holds_alternative<Expr>(expr_);
    }

    template <typename Expr>
    [[nodiscard]] constexpr const Expr* get_if() const noexcept {
        return std::get_if<Expr>(&expr_);
    }

    template <const auto& expr>
    [[nodiscard]] static constexpr const auto& static_visit() noexcept {
        return static_visit_v<expr.expr_>;
    }
};

// These types need to be defined as a classes rather than aliases so that
// they can refer to themselves through their pointer types.
class expr_t : public basic_expr_t<expr_t> {
public:
    using basic_expr_t::basic_expr_t;
    constexpr ~expr_t() noexcept = default;
};

class flat_expr_t : public basic_expr_t<flat_expr_t> {
public:
    using basic_expr_t::basic_expr_t;
    constexpr ~flat_expr_t() noexcept = default;
};

template <typename Expr>
constexpr expr_ptr make_expr(Expr&& expr) {
    return std::make_unique<expr_t>(std::forward<Expr>(expr));
}

}  // namespace ctlox::v2