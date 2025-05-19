#pragma once

#include "types.h"

namespace ctlox {

template <typename... Entries>
struct environment_ct;

struct environment_base {
    // TODO: maybe reduce templates?
    template <string_ct name, typename Value>
    struct entry { };

    template <string_ct name, typename Value, typename... Entries>
    struct define_impl {
        static_assert(false, "logic error: unhandled case");
    };

    template <string_ct name, typename Value>
    struct define_impl<name, Value> {
        // Not found
        template <typename... Entries>
        using f = environment_ct<Entries..., entry<name, Value>>;
    };

    template <string_ct name, typename Value, typename Entry, typename... Entries>
    struct define_impl<name, Value, Entry, Entries...> {
        // searching
        template <typename... Entries2>
        using f = define_impl<name, Value, Entries...>::template f<Entries2..., Entry>;
    };

    template <string_ct name, typename Value, typename OldValue, typename... Entries>
    struct define_impl<name, Value, entry<name, OldValue>, Entries...> {
        // found: replace value
        template <typename... Entries2>
        using f = environment_ct<Entries2..., entry<name, Value>, Entries...>;
    };

    // returns modified environment
    template <string_ct name, typename Value, typename... Entries>
    using define = define_impl<name, Value, Entries...>::template f<>;

    template <string_ct name, typename Value, typename... Entries>
    struct assign_impl {
        static_assert(false, "logic error: unhandled case");
    };

    template <string_ct name, typename Value>
    struct assign_impl<name, Value> {
        // Not found
        static_assert(false, "Undefined variable.");
    };

    template <string_ct name, typename Value, typename Entry, typename... Entries>
    struct assign_impl<name, Value, Entry, Entries...> {
        // searching
        template <typename... Entries2>
        using f = assign_impl<name, Value, Entries...>::template f<Entries2..., Entry>;
    };

    template <string_ct name, typename Value, typename OldValue, typename... Entries>
    struct assign_impl<name, Value, entry<name, OldValue>, Entries...> {
        // found: replace value
        template <typename... Entries2>
        using f = environment_ct<Entries2..., entry<name, Value>, Entries...>;
    };

    // returns modified environment, errors if name not found
    template <string_ct name, typename Value, typename... Entries>
    using assign = assign_impl<name, Value, Entries...>::template f<>;

    template <string_ct name, typename... Entries>
    struct get_impl {
        static_assert(false, "logic error: unhandled case");
    };

    template <string_ct name>
    struct get_impl<name> {
        static_assert(false, "not found");
    };

    template <string_ct name, typename Entry, typename... Entries>
    struct get_impl<name, Entry, Entries...> {
        // searching
        using f = get_impl<name, Entries...>::f;
    };

    template <string_ct name, typename Value, typename... Entries>
    struct get_impl<name, entry<name, Value>, Entries...> {
        // found: return value
        using f = Value;
    };

    // returns the entry, error if not found
    template <string_ct name, typename... Entries>
    using get = get_impl<name, Entries...>::f;
};

template <typename... Entries>
struct environment_ct : private environment_base {
    template <string_ct name, typename Value>
    using define = environment_base::define<name, Value, Entries...>;

    template <string_ct name, typename Value>
    using assign = environment_base::assign<name, Value, Entries...>;

    template <string_ct name>
    using get = environment_base::get<name, Entries...>;
};

struct interpret_ct {
private:
    struct end_of_program { };

    template <typename T>
    static constexpr bool is_truthy(T value)
    {
        if constexpr (std::same_as<T, nil_t>)
            return false;
        if constexpr (std::same_as<T, bool>)
            return value;
        return true; // Strings and numbers are always truthy
    }

    template <typename Left, typename Right>
    static constexpr bool equals(const Left& left, const Right& right)
    {
        if constexpr (std::same_as<Left, Right>) {
            return left == right;
        } else {
            return false;
        }
    }

    static constexpr double plus(const double& left, const double& right)
    {
        return left + right;
    }

    template <std::size_t _left, std::size_t _right>
    static constexpr auto plus(const string_ct<_left>& left, const string_ct<_right>& right)
    {
        return concat(left, right);
    }

    template <typename Left, typename Right>
    static constexpr auto plus(const Left&, const Right&)
    {
        static_assert(false, "Operands must be two numbers or two strings.");
        return nil;
    }

    struct evaluate;

    template <typename Operator>
    struct apply_unary_op {
        template <typename T>
        static constexpr auto apply(T value)
        {
            if constexpr (Operator::type == token_type::bang) {
                return !is_truthy(value);
            } else if constexpr (Operator::type == token_type::minus) {
                static_assert(std::same_as<T, double>);
                return -value;
            } else {
                static_assert(false, "logic error: unhandled case");
                return nil;
            }
        }

        using has_fn = void;
        template <typename C, typename Environment, typename Value, typename... Ts>
        using fn = calln<C, Environment, value_t<apply(Value::value)>, Ts...>;
    };

    template <typename LeftValue, typename Operator>
    struct apply_binary_op {
        template <typename T, typename U>
        static constexpr auto apply(T left, U right)
        {
            if constexpr (Operator::type == token_type::greater) {
                static_assert(std::same_as<decltype(left), double>);
                static_assert(std::same_as<decltype(right), double>);
                return left > right;
            }

            if constexpr (Operator::type == token_type::greater_equal) {
                static_assert(std::same_as<decltype(left), double>);
                static_assert(std::same_as<decltype(right), double>);
                return left >= right;
            }

            if constexpr (Operator::type == token_type::less) {
                static_assert(std::same_as<decltype(left), double>);
                static_assert(std::same_as<decltype(right), double>);
                return left < right;
            }

            if constexpr (Operator::type == token_type::less_equal) {
                static_assert(std::same_as<decltype(left), double>);
                static_assert(std::same_as<decltype(right), double>);
                return left <= right;
            }

            if constexpr (Operator::type == token_type::equal_equal) {
                return equals(left, right);
            }

            if constexpr (Operator::type == token_type::bang_equal) {
                return equals(left, right);
            }

            if constexpr (Operator::type == token_type::minus) {
                static_assert(std::same_as<decltype(left), double>);
                static_assert(std::same_as<decltype(right), double>);
                return left - right;
            }

            if constexpr (Operator::type == token_type::plus) {
                return plus(left, right);
            }

            if constexpr (Operator::type == token_type::slash) {
                static_assert(std::same_as<decltype(left), double>);
                static_assert(std::same_as<decltype(right), double>);
                return left / right;
            }

            if constexpr (Operator::type == token_type::star) {
                static_assert(std::same_as<decltype(left), double>);
                static_assert(std::same_as<decltype(right), double>);
                return left * right;
            }
        }

        using has_fn = void;
        template <typename C, typename Environment, typename RightValue, typename... Ts>
        using fn = calln<C, Environment, value_t<apply(LeftValue::value, RightValue::value)>, Ts...>;
    };

    template <typename Operator, typename RightExpr>
    struct evaluate_right {
        using has_fn = void;
        template <typename C, typename Environment, typename LeftValue, typename... Ts>
        using fn = calln<
            compose<evaluate, apply_binary_op<LeftValue, Operator>, C>,
            Environment, RightExpr, Ts...>;
    };

    template <typename Name>
    struct apply_assign_op {
        using has_fn = void;
        template <typename C, typename Environment, typename Value, typename... Ts>
        using fn = calln<C, typename Environment::template assign<Name::lexeme, Value>, Value, Ts...>;
    };

    struct evaluate {
        template <typename Expr>
        struct impl {
            static_assert(false, "logic error: unhandled case");
        };

        template <auto _value>
        struct impl<literal_expr<_value>> {
            template <typename C, typename Environment, typename... Ts>
            using f = calln<C, Environment, value_t<_value>, Ts...>;
        };

        template <typename Name>
        struct impl<variable_expr<Name>> {
            template <typename C, typename Environment, typename... Ts>
            using f = calln<C, Environment, typename Environment::template get<Name::lexeme>, Ts...>;
        };

        template <typename Name, typename ValueExpr>
        struct impl<assign_expr<Name, ValueExpr>> {
            template <typename C, typename Environment, typename... Ts>
            using f = calln<
                compose<evaluate, apply_assign_op<Name>, C>,
                Environment, ValueExpr, Ts...>;
        };

        template <typename Expr>
        struct impl<grouping_expr<Expr>> {
            template <typename C, typename Environment, typename... Ts>
            using f = calln<compose<evaluate, C>, Environment, Expr, Ts...>;
        };

        template <typename Operator, typename Expr>
        struct impl<unary_expr<Operator, Expr>> {
            template <typename C, typename Environment, typename... Ts>
            using f = calln<
                compose<evaluate, apply_unary_op<Operator>, C>,
                Environment, Expr, Ts...>;
        };

        template <typename LeftExpr, typename Operator, typename RightExpr>
        struct impl<binary_expr<LeftExpr, Operator, RightExpr>> {
            template <typename C, typename Environment, typename... Ts>
            using f = calln<
                compose<evaluate, evaluate_right<Operator, RightExpr>, C>,
                Environment, LeftExpr, Ts...>;
        };

        using has_fn = void;
        template <typename C, typename Environment, typename Expr, typename... Ts>
        using fn = impl<Expr>::template f<C, Environment, Ts...>;
    };

    template <string_ct name>
    struct bind_variable {
        using has_fn = void;
        template <typename C, typename Environment, typename Value, typename... Ts>
        using fn = calln<C, typename Environment::template define<name, Value>, Ts...>;
    };

    struct execute {
        using ignore_result = drop_at<1>;
        using append_result = move_to_back<1>;

        template <typename Stmt>
        struct impl {
            static_assert(false, "logic error: unhandled case");
        };

        template <typename Expr>
        struct impl<expression_stmt<Expr>> {
            template <typename C, typename Environment, typename... Ts>
            using f = calln<compose<evaluate, ignore_result, C>, Environment, Expr, Ts...>;
        };

        template <typename Expr>
        struct impl<print_stmt<Expr>> {
            template <typename C, typename Environment, typename... Ts>
            using f = calln<compose<evaluate, append_result, C>, Environment, Expr, Ts...>;
        };

        template <typename Name, typename Initialiser>
        struct impl<var_stmt<Name, Initialiser>> {
            template <typename C, typename Environment, typename... Ts>
            using f = calln<compose<evaluate, bind_variable<Name::lexeme>, C>, Environment, Initialiser, Ts...>;
        };

        using has_fn = void;
        template <typename C, typename Environment, typename Stmt, typename... Ts>
        using fn = impl<Stmt>::template f<C, Environment, Ts...>;
    };

    struct interpret {
        template <bool is_eop>
        struct impl {
            template <typename C, typename Environment, typename... Ts>
            using f = calln<compose<execute, interpret, C>, Environment, Ts...>;
        };

        template <>
        struct impl<true> {
            // found sentinel: done interpreting
            template <typename C, typename Environment, typename EndOfProgram, typename... Output>
            using f = calln<C, Output...>;
        };

        using has_fn = void;
        template <accepts_pack C, typename Environment, typename T0, typename... Ts>
        using fn = impl<std::is_same_v<T0, end_of_program>>::template f<C, Environment, T0, Ts...>;
    };

public:
    using has_fn = void;
    template <accepts_pack C, typename... Statements>
    using fn = calln<
        compose<interpret, C>,
        environment_ct<>, Statements..., end_of_program>;
};
}

#include "parser_ct.h"
#include "scanner_ct.h"

namespace ctlox::interpreter_tests {
template <string_ct s, auto _expected>
constexpr bool test()
{
    using program_output = run<
        scan_ct<s>,
        parse_ct,
        interpret_ct,
        at<0>,
        returned>;
    if constexpr (_expected == program_output::value) {
        return true;
    } else {
        using output = run<
            given_pack<value_t<_expected>, typename program_output::type>,
            errored>;
        return false;
    }
}

static_assert(concat("hello"_ct, ", "_ct, "world!"_ct) == "hello, world!"_ct);

constexpr bool r0 = test<"print 1;", 1.0>();
constexpr bool r1 = test<"print -5;", -5.0>();
constexpr bool r2 = test<R"(print !!!!"hello";)", true>();
constexpr bool r3 = test<R"(print (nil);)", nil>();
constexpr bool r4 = test<R"(print 4 <= 4;)", true>();
constexpr bool r5 = test<R"(print 4 < 3;)", false>();
constexpr bool r6 = test<"print 5 * 100 / 22;", 5.0 * 100.0 / 22.0>();
constexpr bool r7 = test<"print 5 * (100 / 22);", 5.0 * (100.0 / 22.0)>();
constexpr bool r8 = test<"var foo; var bar; foo = (bar = 2) + 5; print foo;", 7.0>();
}