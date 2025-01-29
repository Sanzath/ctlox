#pragma once

#include "ct.h"


namespace ctlox {
    template <auto _literal>
    struct literal_expr {};

    template <typename Operator, typename Right>
    struct unary_expr {};

    template <typename Left, typename Operator, typename Right>
    struct binary_expr {};

    template <typename Expr>
    struct grouping_expr {};

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
                requires (T::type == token_type::right_paren)
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
                requires (Token::literal != none)
            struct impl<Token, Tokens...> {
                template <typename C>
                using f = calln<C, literal_expr<Token::literal>, Tokens...>;
            };

            template <typename Token, typename... Tokens>
                requires (Token::type == token_type::left_paren)
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
            struct impl {};

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
            struct impl : consume_primary::impl<Tokens...> {
            };

            template <typename Token, typename... Tokens>
                requires (Token::type == token_type::bang || Token::type == token_type::minus)
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

        //struct maybe_match_equality {
        //   // base case: just call C with everything
        // 
        //   template <typename E, typename T, typename... Ts>
        //       requires (Token::type == token_type::equal_equal || Token::type == token_type::bang_equal)
        //   struct impl<E, Token, Tokens...> {
        //       template <typename C>
        //       using fn = calln<
        //           compose<consume_unary, bind_binary_expr<E, Token>, maybe_match_equality, C>,
        //           Tokens...>;
        //   };
        //   
        //    using has_fn = void;
        //    template <typename C, typename... Ts>
        //    using fn = impl<Ts...>::template f<C>;
        //};
        using maybe_match_equality = noop;

        using consume_factor = compose<consume_unary, maybe_match_equality>;

        // using compositions works, but inheriting them does not.
        // because cont_traits does not see consume_expression as a composition<...>.
        using consume_expression = consume_factor;

        struct consume_expression_workaround { using type = consume_expression; };

    public:
        using has_fn = void;
        template <accepts_pack C, typename... Tokens>
        using fn = calln<compose<consume_expression, C>, Tokens...>;
    };

    namespace parse_tests {
        static_assert(none == none);
        static_assert(none != 154.0);

        template <string_ct s, typename Expr>
        constexpr inline bool test = std::same_as<
            Expr,
            run<scan_ct<s>, parse_ct, at<0>, returned>>;

        using t0 = run<
            given_pack<
            token_ct<0, token_type::number, "23.52", 123.52>
            >,
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
            unary_expr<token_ct<0, token_type::bang, "!">, literal_expr<123.52>>
        >);
        static_assert(test<
            "-false",
            unary_expr<token_ct<0, token_type::minus, "-">, literal_expr<false>>
        >);
        static_assert(test<
            "!!true",
            unary_expr<token_ct<0, token_type::bang, "!">, 
            unary_expr<token_ct<1, token_type::bang, "!">, literal_expr<true>>>
        >);
        //using t1 = run<
        //    given_pack<
        //    token_ct<0, token_type::bang, "!">,
        //    token_ct<1, token_type::number, "23.52", 123.52>
        //    >,
        //    parse_ct,
        //    returned>;
        //static_assert(std::same_as<t1,
        //    unary_expr<
        //    token_ct<0, token_type::bang, "!">,
        //    literal_expr<123.52>
        //    >>);
    }
}