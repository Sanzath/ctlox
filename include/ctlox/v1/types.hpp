#pragma once

#include <ctlox/common/string.hpp>
#include <ctlox/common/token_type.hpp>

namespace ctlox::v1 {
struct none_t {
    constexpr bool operator==(const none_t&) const noexcept { return true; }

    template <typename T>
    constexpr bool operator==(const T&) const noexcept { return false; }
};

struct nil_t {
    constexpr bool operator==(const nil_t&) const noexcept { return true; }

    template <typename T>
    constexpr bool operator==(const T&) const noexcept { return false; }
};

static constexpr inline none_t none;  // not a literal
static constexpr inline nil_t nil;  // nil literal

template <auto _value>
struct value_t {
    using type = value_t;
    using value_type = decltype(_value);
    static constexpr inline value_type value = _value;

    constexpr value_type operator()() const noexcept { return value; }
    constexpr explicit(false) operator value_type() const noexcept { return value; }  // NOLINT(google-explicit-constructor)
};

template <std::size_t _location, token_type _type, string _lexeme, auto _literal = none>
struct token_t {
    static constexpr inline auto location = _location;
    static constexpr inline auto type = _type;
    static constexpr inline auto lexeme = _lexeme;
    static constexpr inline auto literal = _literal;
};

// expression template-style AST types:

// statements
template <typename Name, typename InitializerExpr>
struct var_stmt { };

template <typename Expr>
struct expression_stmt { };

template <typename Expr>
struct print_stmt { };

template <typename... Statements>
struct block_stmt { };

// expressions
template <auto _literal>
struct literal_expr { };

template <typename Name>
struct variable_expr { };

template <typename Name, typename ValueExpr>
struct assign_expr { };

template <typename Operator, typename Right>
struct unary_expr { };

template <typename Left, typename Operator, typename Right>
struct binary_expr { };

template <typename Expr>
struct grouping_expr { };

}  // namespace ctlox::v1