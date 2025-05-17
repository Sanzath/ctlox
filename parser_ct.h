#pragma once

#include "ct.h"

#include "types.h"

namespace ctlox {
struct parse_ct {
private:
    // struct expression
    // struct equality
    // struct comparison
    // struct term
    // struct factor
    // struct unary

    struct consume_right_paren {
        template <typename... Ts>
        struct impl {
            // is there a way to error out without a hard error?
            // With tokens, we could just intersperse the errors in the tokens.
            // But that doesn't really work here.
            static_assert(false, "Expect ')' after expression.");
        };

        template <typename E, typename T, typename... Ts>
            requires(T::type == token_type::right_paren)
        struct impl<E, T, Ts...> {
            template <typename C>
            using f = calln<C, grouping_expr<E>, Ts...>;
        };

        using has_fn = void;
        template <typename C, typename... Ts>
        using fn = impl<Ts...>::template f<C>;
    };

    struct consume_expression_workaround;

    struct consume_primary {
        template <typename... Ts>
        struct impl {
            static_assert(false, "Expect expression.");
        };

        template <typename Token, typename... Tokens>
            requires(Token::literal != none)
        struct impl<Token, Tokens...> {
            template <typename C>
            using f = calln<C, literal_expr<Token::literal>, Tokens...>;
        };

        template <typename Token, typename... Tokens>
            requires(Token::type == token_type::left_paren)
        struct impl<Token, Tokens...> {
            template <typename C>
            using f = calln<
                compose<typename dcall<consume_expression_workaround, sizeof(C)>::type, consume_right_paren, C>,
                Tokens...>;
        };

        using has_fn = void;
        template <typename C, typename... Ts>
        using fn = impl<Ts...>::template f<C>;
    };

    template <typename Token>
    struct bind_unary_expr {
        template <typename... Ts>
        struct impl { };

        template <typename E, typename... Tokens>
        struct impl<E, Tokens...> {
            template <typename C>
            using f = calln<C, unary_expr<Token, E>, Tokens...>;
        };

        using has_fn = void;
        template <typename C, typename... Ts>
        using fn = impl<Ts...>::template f<C>;
    };

    struct consume_unary {
        template <typename... Tokens>
        struct impl : consume_primary::impl<Tokens...> { };

        template <typename Token, typename... Tokens>
            requires(Token::type == token_type::bang || Token::type == token_type::minus)
        struct impl<Token, Tokens...> {
            template <typename C>
            using f = calln<
                compose<consume_unary, bind_unary_expr<Token>, C>,
                Tokens...>;
        };

        using has_fn = void;
        template <typename C, typename... Ts>
        using fn = impl<Ts...>::template f<C>;
    };

    template <typename LeftExpr, typename Token>
    struct bind_binary_expr {
        template <typename... Ts>
        struct impl { };

        template <typename RightExpr, typename... Tokens>
        struct impl<RightExpr, Tokens...> {
            template <typename C>
            using f = calln<C, binary_expr<LeftExpr, Token, RightExpr>, Tokens...>;
        };

        using has_fn = void;
        template <typename C, typename... Ts>
        using fn = impl<Ts...>::template f<C>;
    };

    struct maybe_match_product_op {
        template <typename... Ts>
        struct impl {
            template <typename C>
            using f = calln<C, Ts...>;
        };

        template <typename E, typename Token, typename... Tokens>
            requires(Token::type == token_type::slash || Token::type == token_type::star)
        struct impl<E, Token, Tokens...> {
            template <typename C>
            using f = calln<
                compose<consume_unary, bind_binary_expr<E, Token>, maybe_match_product_op, C>,
                Tokens...>;
        };

        using has_fn = void;
        template <typename C, typename... Ts>
        using fn = impl<Ts...>::template f<C>;
    };

    using consume_factor = compose<consume_unary, maybe_match_product_op>;

    struct maybe_match_sum_op {
        template <typename... Ts>
        struct impl {
            template <typename C>
            using f = calln<C, Ts...>;
        };

        template <typename E, typename Token, typename... Tokens>
            requires(Token::type == token_type::plus || Token::type == token_type::minus)
        struct impl<E, Token, Tokens...> {
            template <typename C>
            using f = calln<
                compose<consume_factor, bind_binary_expr<E, Token>, maybe_match_sum_op, C>,
                Tokens...>;
        };

        using has_fn = void;
        template <typename C, typename... Ts>
        using fn = impl<Ts...>::template f<C>;
    };

    using consume_term = compose<consume_factor, maybe_match_sum_op>;

    static constexpr inline bool is_comparison_op(token_type type)
    {
        return type == token_type::greater
            || type == token_type::greater_equal
            || type == token_type::less
            || type == token_type::less_equal;
    }

    struct maybe_match_comparison_op {
        template <typename... Ts>
        struct impl {
            template <typename C>
            using f = calln<C, Ts...>;
        };

        template <typename E, typename Token, typename... Tokens>
            requires(is_comparison_op(Token::type))
        struct impl<E, Token, Tokens...> {
            template <typename C>
            using f = calln<
                compose<consume_term, bind_binary_expr<E, Token>, maybe_match_comparison_op, C>,
                Tokens...>;
        };

        using has_fn = void;
        template <typename C, typename... Ts>
        using fn = impl<Ts...>::template f<C>;
    };

    using consume_comparison = compose<consume_term, maybe_match_comparison_op>;

    struct maybe_match_equality_op {
        template <typename... Ts>
        struct impl {
            template <typename C>
            using f = calln<C, Ts...>;
        };

        template <typename E, typename Token, typename... Tokens>
            requires(Token::type == token_type::equal_equal || Token::type == token_type::bang_equal)
        struct impl<E, Token, Tokens...> {
            template <typename C>
            using f = calln<
                compose<consume_comparison, bind_binary_expr<E, Token>, maybe_match_equality_op, C>,
                Tokens...>;
        };

        using has_fn = void;
        template <typename C, typename... Ts>
        using fn = impl<Ts...>::template f<C>;
    };

    using consume_equality = compose<consume_comparison, maybe_match_equality_op>;

    // TODO: eliminate leftover non-expressions?
    // TODO: synchronize? Not sure if that's feasible or even desirable here.
    using consume_expression = consume_equality;

    struct consume_expression_workaround {
        using type = consume_expression;
    };

public:
    using has_fn = void;
    template <accepts_pack C, typename... Tokens>
    using fn = calln<compose<consume_expression, C>, Tokens...>;
};
}

#include "scanner_ct.h"

namespace ctlox::parse_tests {
    static_assert(none == none);
    static_assert(none != 154.0);

    template <string_ct s, typename Expr>
    constexpr inline bool test = std::same_as<
        Expr,
        run<scan_ct<s>, parse_ct, at<0>, returned>>;
    template <string_ct s>
    using debugtest = run<scan_ct<s>, parse_ct, at<0>, errored>;

    using t0 = run<
        given_pack<
            token_ct<0, token_type::number, "23.52", 123.52>>,
        parse_ct,
        at<0>,
        returned>;
    static_assert(std::same_as<t0, literal_expr<123.52>>);

    using t1 = run<
        scan_ct<"(123.52)">,
        parse_ct,
        at<0>,
        returned>;
    static_assert(std::same_as<t1, grouping_expr<literal_expr<123.52>>>);

    static_assert(test<
        "!123.52",
        unary_expr<token_ct<0, token_type::bang, "!">, literal_expr<123.52>>>);
    static_assert(test<
        "-false",
        unary_expr<token_ct<0, token_type::minus, "-">, literal_expr<false>>>);
    static_assert(test<
        "!!true",
        unary_expr<token_ct<0, token_type::bang, "!">,
            unary_expr<token_ct<1, token_type::bang, "!">, literal_expr<true>>>>);
    static_assert(test<
        "2 * 3",
        binary_expr<
            literal_expr<2.0>,
            token_ct<2, token_type::star, "*">,
            literal_expr<3.0>>>);
    static_assert(test<
        "1 * -2 / !false",
        binary_expr<
            binary_expr<
                literal_expr<1.0>,
                token_ct<2, token_type::star, "*">,
                unary_expr<token_ct<4, token_type::minus, "-">, literal_expr<2.0>>>,
            token_ct<7, token_type::slash, "/">,
            unary_expr<token_ct<9, token_type::bang, "!">, literal_expr<false>>>>);
    static_assert(test<
        R"(13.3 + "stuff")",
        binary_expr<
            literal_expr<13.3>,
            token_ct<5, token_type::plus, "+">,
            literal_expr<"stuff"_ct>>>);
    static_assert(test<
        R"(nil - nil - nil)",
        binary_expr<
            binary_expr<
                literal_expr<nil>,
                token_ct<4, token_type::minus, "-">,
                literal_expr<nil>>,
            token_ct<10, token_type::minus, "-">,
            literal_expr<nil>>>);
    static_assert(test<
        "1 + 2 * 3 - 4",
        binary_expr<
            binary_expr<
                literal_expr<1.0>,
                token_ct<2, token_type::plus, "+">,
                binary_expr<
                    literal_expr<2.0>,
                    token_ct<6, token_type::star, "*">,
                    literal_expr<3.0>>>,
            token_ct<10, token_type::minus, "-">,
            literal_expr<4.0>>>);
    static_assert(test<"1 * 2 + 3 / 4",
        binary_expr<
            binary_expr<
                literal_expr<1.0>,
                token_ct<2, token_type::star, "*">,
                literal_expr<2.0>>,
            token_ct<6, token_type::plus, "+">,
            binary_expr<
                literal_expr<3.0>,
                token_ct<10, token_type::slash, "/">,
                literal_expr<4.0>>>>);
    static_assert(test<"true > nil",
        binary_expr<literal_expr<true>, token_ct<5, token_type::greater, ">">, literal_expr<nil>>>);
    static_assert(test<"1 > 2 <= 3 >= 0",
        binary_expr<
            binary_expr<
                binary_expr<
                    literal_expr<1.0>,
                    token_ct<2, token_type::greater, ">">,
                    literal_expr<2.0>>,
                token_ct<6, token_type::less_equal, "<=">,
                literal_expr<3.0>>,
            token_ct<11, token_type::greater_equal, ">=">,
            literal_expr<0.0>>>);
    static_assert(test<"nil != nil == false",
        binary_expr<
            binary_expr<
                literal_expr<nil>,
                token_ct<4, token_type::bang_equal, "!=">,
                literal_expr<nil>>,
            token_ct<11, token_type::equal_equal, "==">,
            literal_expr<false>>>);
    static_assert(test<"7 == 2 * 3.5",
        binary_expr<
            literal_expr<7.0>,
            token_ct<2, token_type::equal_equal, "==">,
            binary_expr<
                literal_expr<2.0>,
                token_ct<7, token_type::star, "*">,
                literal_expr<3.5>>>>);
    static_assert(test<"(7 == 2) * 3.5",
        binary_expr<
            grouping_expr<
                binary_expr<
                    literal_expr<7.0>,
                    token_ct<3, token_type::equal_equal, "==">,
                    literal_expr<2.0>>>,
            token_ct<9, token_type::star, "*">,
            literal_expr<3.5>>>);
}