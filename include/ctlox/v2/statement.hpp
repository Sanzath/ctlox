#pragma once

#include <ctlox/v2/expression.hpp>
#include <ctlox/v2/token.hpp>

#include <memory>
#include <variant>
#include <vector>

namespace ctlox::v2 {

class stmt_t;
using stmt_ptr_t = std::unique_ptr<stmt_t>;

struct block_stmt {
    std::vector<stmt_ptr_t> statements_;
};

struct expression_stmt {
    expr_ptr_t expression_;
};

struct print_stmt {
    expr_ptr_t expression_;
};

struct var_stmt {
    token_t name_;
    expr_ptr_t initializer_;
};

class stmt_t final {
    using variant_t = std::variant<
        block_stmt,
        expression_stmt,
        print_stmt,
        var_stmt>;

    variant_t stmt_;

public:
    template <typename Stmt>
        requires std::constructible_from<variant_t, Stmt&&>
    constexpr explicit(false) stmt_t(Stmt&& stmt)
        : stmt_(std::forward<Stmt>(stmt)) { }

    constexpr ~stmt_t() = default;

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

template <typename Stmt>
constexpr stmt_ptr_t make_stmt(Stmt&& stmt) {
    return std::make_unique<stmt_t>(std::forward<Stmt>(stmt));
}

}  // namespace ctlox::v2