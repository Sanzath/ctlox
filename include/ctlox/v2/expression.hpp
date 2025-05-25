#pragma once

#include <ctlox/v2/token.hpp>

#include <memory>
#include <variant>

namespace ctlox::v2 {

class expr_t;
using expr_ptr_t = std::unique_ptr<expr_t>;

struct assign_expr {
    token_t name_;
    expr_ptr_t value_;
};

struct binary_expr {
    expr_ptr_t left_;
    token_t operator_;
    expr_ptr_t right_;
};

struct grouping_expr {
    expr_ptr_t expr_;
};

struct literal_expr {
    literal_t value_;
};

struct unary_expr {
    token_t operator_;
    expr_ptr_t right_;
};

struct variable_expr {
    token_t name_;
};

class expr_t final {
    using variant_t = std::variant<
        assign_expr,
        binary_expr,
        grouping_expr,
        literal_expr,
        unary_expr,
        variable_expr>;

    variant_t expr_;

public:
    template <typename Expr>
        requires std::constructible_from<variant_t, Expr&&>
    constexpr explicit(false) expr_t(Expr&& expr)
        : expr_(std::forward<Expr>(expr)) { }

    constexpr ~expr_t() = default;

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
};

template <typename Expr>
constexpr expr_ptr_t make_expr(Expr&& expr) {
    return std::make_unique<expr_t>(std::forward<Expr>(expr));
}

}  // namespace ctlox::v2