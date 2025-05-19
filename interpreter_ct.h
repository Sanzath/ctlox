#pragma once

#include "types.h"
#include <utility>
#include <vector>

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

template <typename... Entries>
struct environment_ct;

struct environment_base {
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
    struct sentinel { };

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

        template <typename Environment, typename Value, typename... Ts>
        struct impl {
            template <typename C>
            using f = calln<C, Environment, value_t<apply(Value::value)>, Ts...>;
        };

        using has_fn = void;
        template <typename C, typename... Ts>
        using fn = impl<Ts...>::template f<C>;
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

        template <typename Environment, typename RightValue, typename... Ts>
        struct impl {
            template <typename C>
            using f = calln<C, Environment, value_t<apply(LeftValue::value, RightValue::value)>, Ts...>;
        };

        using has_fn = void;
        template <typename C, typename... Ts>
        using fn = impl<Ts...>::template f<C>;
    };

    template <typename Operator, typename RightExpr>
    struct evaluate_right {
        template <typename Environment, typename LeftValue, typename... Ts>
        struct impl {
            template <typename C>
            using f = calln<
                compose<evaluate, apply_binary_op<LeftValue, Operator>, C>,
                Environment, RightExpr, Ts...>;
        };

        using has_fn = void;
        template <typename C, typename... Ts>
        using fn = impl<Ts...>::template f<C>;
    };

    template <typename Name>
    struct apply_assign_op {
        template <typename Environment, typename Value, typename... Ts>
        struct impl {
            template <typename C>
            using f = calln<C, typename Environment::template assign<Name::lexeme, Value>, Value, Ts...>;
        };

        using has_fn = void;
        template <typename C, typename... Ts>
        using fn = impl<Ts...>::template f<C>;
    };

    struct evaluate {
        template <typename Environment, typename... Ts>
        struct impl {
            static_assert(false, "logic error: unhandled case");
        };

        template <typename Environment, auto _value, typename... Ts>
        struct impl<Environment, literal_expr<_value>, Ts...> {
            template <typename C>
            using f = calln<C, Environment, value_t<_value>, Ts...>;
        };

        template <typename Environment, typename Name, typename... Ts>
        struct impl<Environment, variable_expr<Name>, Ts...> {
            template <typename C>
            using f = calln<C, Environment, typename Environment::template get<Name::lexeme>, Ts...>;
        };

        template <typename Environment, typename Name, typename ValueExpr, typename... Ts>
        struct impl<Environment, assign_expr<Name, ValueExpr>, Ts...> {
            template <typename C>
            using f = calln<
                compose<evaluate, apply_assign_op<Name>, C>,
                Environment, ValueExpr, Ts...>;
        };

        template <typename Environment, typename Expr, typename... Ts>
        struct impl<Environment, grouping_expr<Expr>, Ts...> {
            template <typename C>
            using f = calln<compose<evaluate, C>, Environment, Expr, Ts...>;
        };

        template <typename Environment, typename Operator, typename Expr, typename... Ts>
        struct impl<Environment, unary_expr<Operator, Expr>, Ts...> {
            template <typename C>
            using f = calln<
                compose<evaluate, apply_unary_op<Operator>, C>,
                Environment, Expr, Ts...>;
        };

        template <typename Environment, typename LeftExpr, typename Operator, typename RightExpr, typename... Ts>
        struct impl<Environment, binary_expr<LeftExpr, Operator, RightExpr>, Ts...> {
            template <typename C>
            using f = calln<
                compose<evaluate, evaluate_right<Operator, RightExpr>, C>,
                Environment, LeftExpr, Ts...>;
        };

        using has_fn = void;
        template <typename C, typename... Ts>
        using fn = impl<Ts...>::template f<C>;
    };

    template <string_ct name>
    struct bind_variable {
        template <typename Environment, typename Value, typename... Ts>
        struct impl {
            template <typename C>
            using f = calln<C, typename Environment::template define<name, Value>, Ts...>;
        };

        using has_fn = void;
        template <typename C, typename... Ts>
        using fn = impl<Ts...>::template f<C>;
    };

    struct execute {
        using ignore_result = drop_at<1>;
        using append_result = move_to_back<1>;

        template <typename... Ts>
        struct impl {
            static_assert(false, "logic error: unhandled case");
        };

        template <typename Environment, typename Expr, typename... Ts>
        struct impl<Environment, expression_stmt<Expr>, Ts...> {
            template <typename C>
            using f = calln<compose<evaluate, ignore_result, C>, Environment, Expr, Ts...>;
        };

        template <typename Environment, typename Expr, typename... Ts>
        struct impl<Environment, print_stmt<Expr>, Ts...> {
            template <typename C>
            using f = calln<compose<evaluate, append_result, C>, Environment, Expr, Ts...>;
        };

        template <typename Environment, typename Name, typename Initialiser, typename... Ts>
        struct impl<Environment, var_stmt<Name, Initialiser>, Ts...> {
            template <typename C>
            using f = calln<compose<evaluate, bind_variable<Name::lexeme>, C>, Environment, Initialiser, Ts...>;
        };

        using has_fn = void;
        template <typename C, typename... Ts>
        using fn = impl<Ts...>::template f<C>;
    };

    struct interpret {
        template <typename... Ts>
        struct impl {
            template <typename C>
            using f = calln<compose<execute, interpret, C>, Ts...>;
        };

        template <typename Environment, typename... Output>
        struct impl<Environment, sentinel, Output...> {
            // found sentinel: done interpreting
            template <typename C>
            using f = calln<C, Output...>;
        };

        using has_fn = void;
        template <accepts_pack C, typename... Ts>
        using fn = impl<Ts...>::template f<C>;
    };

public:
    using has_fn = void;
    template <accepts_pack C, typename... Statements>
    using fn = calln<
        compose<interpret, C>,
        environment_ct<>, Statements..., sentinel>;
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