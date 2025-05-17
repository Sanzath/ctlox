#pragma once

#include "types.h"

namespace ctlox {
struct interpreter {
    template <typename Expr>
    constexpr auto interpret(Expr expr)
    {
        return this->evaluate(expr);
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

struct interpret_ct {
    using has_f1 = void;

    template <accepts_one C, typename Expr>
    using f1 = call1<C, value_t<interpreter{}.interpret(Expr{})>>;
};
}

#include "parser_ct.h"
#include "scanner_ct.h"

namespace ctlox::interpreter_tests {
template <auto _value>
struct value_t { };

template <string_ct s, auto _expected>
constexpr bool test()
{
    using expr = run<
        scan_ct<s>,
        parse_ct,
        at<0>,
        returned>;
    constexpr auto actual = interpreter().interpret(expr {});
    if constexpr (_expected == actual) {
        return true;
    } else {
        using output = run<
            given_pack<value_t<_expected>, value_t<actual>>,
            errored>;
        return false;
    }
}

static_assert(concat("hello"_ct, ", "_ct, "world!"_ct) == "hello, world!"_ct);

constexpr bool r0 = test<"1", 1.0>();
constexpr bool r1 = test<"-5", -5.0>();
constexpr bool r2 = test<R"(!!!!"hello")", true>();
constexpr bool r3 = test<R"((nil))", nil>();
constexpr bool r4 = test<R"(4 <= 4)", true>();
constexpr bool r5 = test<R"(4 < 3)", false>();
constexpr bool r6 = test<"5 * 100 / 22", 5.0 * 100.0 / 22.0>();
constexpr bool r7 = test<"5 * (100 / 22)", 5.0 * (100.0 / 22.0)>();
}