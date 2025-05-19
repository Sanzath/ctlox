#pragma once

#include "types.h"
#include <tuple>
#include <utility>

namespace ctlox {
struct interpreter {
    template <typename... Statements>
    constexpr auto interpret(Statements... statements)
    {
        return std::tuple_cat(this->execute(statements)...);
    }

    template <typename Expr>
    constexpr std::tuple<> execute(expression_stmt<Expr>)
    {
        this->evaluate(Expr {});
        return {};
    }

    template <typename Expr>
    constexpr auto execute(print_stmt<Expr>)
    {
        return std::tuple(this->evaluate(Expr {}));
    }

    template <auto _literal>
    constexpr auto evaluate(literal_expr<_literal>)
    {
        return _literal;
    }

    template <typename Expr>
    constexpr auto evaluate(grouping_expr<Expr>)
    {
        return this->evaluate(Expr {});
    }

    template <typename Token, typename Expr>
    constexpr auto evaluate(unary_expr<Token, Expr>)
    {
        auto right = this->evaluate(Expr {});

        if constexpr (Token::type == token_type::bang) {
            return !this->is_truthy(right);
        } else if constexpr (Token::type == token_type::minus) {
            static_assert(std::same_as<decltype(right), double>);
            return -right;
        } else {
            static_assert(false, "unreachable");
            return nil;
        }
    }

    template <typename LeftExpr, typename Token, typename RightExpr>
    constexpr auto evaluate(binary_expr<LeftExpr, Token, RightExpr>)
    {
        auto left = this->evaluate(LeftExpr {});
        auto right = this->evaluate(RightExpr {});

        if constexpr (Token::type == token_type::greater) {
            static_assert(std::same_as<decltype(left), double>);
            static_assert(std::same_as<decltype(right), double>);
            return left > right;
        }

        if constexpr (Token::type == token_type::greater_equal) {
            static_assert(std::same_as<decltype(left), double>);
            static_assert(std::same_as<decltype(right), double>);
            return left >= right;
        }

        if constexpr (Token::type == token_type::less) {
            static_assert(std::same_as<decltype(left), double>);
            static_assert(std::same_as<decltype(right), double>);
            return left < right;
        }

        if constexpr (Token::type == token_type::less_equal) {
            static_assert(std::same_as<decltype(left), double>);
            static_assert(std::same_as<decltype(right), double>);
            return left <= right;
        }

        if constexpr (Token::type == token_type::equal_equal) {
            return this->equals(left, right);
        }

        if constexpr (Token::type == token_type::bang_equal) {
            return !this->equals(left, right);
        }

        if constexpr (Token::type == token_type::minus) {
            static_assert(std::same_as<decltype(left), double>);
            static_assert(std::same_as<decltype(right), double>);
            return left - right;
        }

        if constexpr (Token::type == token_type::plus) {
            return this->plus(left, right);
        }

        if constexpr (Token::type == token_type::slash) {
            static_assert(std::same_as<decltype(left), double>);
            static_assert(std::same_as<decltype(right), double>);
            return left / right;
        }

        if constexpr (Token::type == token_type::star) {
            static_assert(std::same_as<decltype(left), double>);
            static_assert(std::same_as<decltype(right), double>);
            return left * right;
        }
    }

private:
    template <typename Value>
    constexpr bool is_truthy(const Value& value)
    {
        if constexpr (std::same_as<Value, nil_t>)
            return false;
        if constexpr (std::same_as<Value, bool>)
            return value;
        return true; // Strings and numbers are always truthy
    }

    template <typename Left, typename Right>
    constexpr bool equals(const Left& left, const Right& right)
    {
        if constexpr (std::same_as<Left, Right>) {
            return left == right;
        } else {
            return false;
        }
    }

    constexpr double plus(const double& left, const double& right)
    {
        return left + right;
    }

    template <std::size_t _left, std::size_t _right>
    constexpr auto plus(const string_ct<_left>& left, const string_ct<_right>& right)
    {
        return concat(left, right);
    }

    template <typename Left, typename Right>
    constexpr auto plus(const Left&, const Right&)
    {
        static_assert(false, "Operands must be two numbers or two strings.");
        return nil;
    }
};

}