#pragma once

#include "ct.h"

#include "types.h"

namespace ctlox {
struct parse_ct {
private:
    struct consume_right_paren {
        template <typename... Ts>
        struct impl {
            // TODO: is there a way to error out without a hard error?
            // With tokens, we could just intersperse the errors in the tokens.
            // But that doesn't really work here.
            // -- Actually, by now parsing full statements, something could be done.
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

        template <typename Token, typename... Ts>
            requires(Token::literal != none)
        struct impl<Token, Ts...> {
            template <typename C>
            using f = calln<C, literal_expr<Token::literal>, Ts...>;
        };

        template <typename Token, typename... Ts>
            requires(Token::type == token_type::identifier)
        struct impl<Token, Ts...> {
            template <typename C>
            using f = calln<C, variable_expr<Token>, Ts...>;
        };

        template <typename Token, typename... Ts>
            requires(Token::type == token_type::left_paren)
        struct impl<Token, Ts...> {
            template <typename C>
            using f = calln<
                compose<typename dcall<consume_expression_workaround, sizeof(C)>::type, consume_right_paren, C>,
                Ts...>;
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

    using consume_expression = consume_equality;

    struct consume_expression_workaround {
        using type = consume_expression;
    };

    struct consume_semicolon {
        template <typename... Ts>
        struct impl {
            static_assert(false, "Expect ';' after statement.");
        };

        template <typename E, typename Token, typename... Ts>
            requires(Token::type == token_type::semicolon)
        struct impl<E, Token, Ts...> {
            template <typename C>
            using f = calln<C, E, Ts...>;
        };

        using has_fn = void;
        template <typename C, typename... Ts>
        using fn = impl<Ts...>::template f<C>;
    };

    template <template <typename...> typename Stmt, typename... Args>
    struct add_statement {
        template <typename E, typename... Ts>
        struct impl {
            template <typename C>
            using f = calln<C, Ts..., Stmt<Args..., E>>;
        };

        using has_fn = void;
        template <typename C, typename... Ts>
        using fn = impl<Ts...>::template f<C>;
    };

    struct consume_statement {
        template <typename... Ts>
        struct impl {
            // base case: expression statement
            template <typename C>
            using f = calln<
                compose<consume_expression, consume_semicolon, add_statement<expression_stmt>, C>,
                Ts...>;
        };

        template <typename Token, typename... Ts>
            requires(Token::type == token_type::_print)
        struct impl<Token, Ts...> {
            // print statement
            template <typename C>
            using f = calln<
                compose<consume_expression, consume_semicolon, add_statement<print_stmt>, C>,
                Ts...>;
        };

        using has_fn = void;
        template <accepts_pack C, typename... Tokens>
        using fn = impl<Tokens...>::template f<C>;
    };

    struct consume_declaration {
        template <typename... Ts>
        struct impl : consume_statement::impl<Ts...> { };

        template <typename Token, typename... Ts>
            requires(Token::type == token_type::_var)
        struct impl<Token, Ts...> {
            static_assert(false, "Expect form 'var <name>;' or 'var <name> = <expr>;'.");
        };

        template <typename Token, typename Identifier, typename Semicolon, typename... Ts>
            requires(Token::type == token_type::_var
                && Identifier::type == token_type::identifier
                && Semicolon::type == token_type::semicolon)
        struct impl<Token, Identifier, Semicolon, Ts...> {
            // var declaration without initializer
            template <typename C>
            using f = calln<C, Ts..., var_stmt<Identifier, void>>;
        };

        template <typename Token, typename Identifier, typename Equal, typename... Ts>
            requires(Token::type == token_type::_var
                && Identifier::type == token_type::identifier
                && Equal::type == token_type::equal)
        struct impl<Token, Identifier, Equal, Ts...> {
            // var declaration with initializer
            template <typename C>
            using f = calln<
                compose<consume_expression, consume_semicolon, add_statement<var_stmt, Identifier>, C>,
                Ts...>;
        };

        using has_fn = void;
        template <accepts_pack C, typename... Tokens>
        using fn = impl<Tokens...>::template f<C>;
    };

    struct consume_program {
        template <typename... Ts>
        struct impl {
            template <typename C>
            using f = calln<compose<consume_declaration, consume_program, C>, Ts...>;
        };

        template <typename Token, typename... Statements>
            requires(Token::type == token_type::eof)
        struct impl<Token, Statements...> {
            // found eof: done parsing
            template <typename C>
            using f = calln<C, Statements...>;
        };

        using has_fn = void;
        template <accepts_pack C, typename... Tokens>
        using fn = impl<Tokens...>::template f<C>;
    };

public:
    using has_fn = void;
    template <accepts_pack C, typename... Tokens>
    using fn = calln<compose<consume_program, C>, Tokens...>;
};
}

#include "scanner_ct.h"

namespace ctlox::parse_tests {
static_assert(none == none);
static_assert(none != 154.0);

template <string_ct s, typename... Statements>
constexpr inline bool test = std::same_as<
    list<Statements...>,
    run<scan_ct<s>, parse_ct, listed>>;

template <string_ct s, typename... Statements>
using debugtest = run<scan_ct<s>, parse_ct, errored>;

using t0 = run<
    given_pack<
        token_ct<0, token_type::number, "23.52", 123.52>,
        token_ct<5, token_type::semicolon, ";">,
        token_ct<6, token_type::eof, "">>,
    parse_ct,
    at<0>,
    returned>;
static_assert(std::same_as<t0, expression_stmt<literal_expr<123.52>>>);

static_assert(test<
    "(123.52);",
    expression_stmt<grouping_expr<literal_expr<123.52>>>>);
static_assert(test<
    "!123.52;",
    expression_stmt<unary_expr<token_ct<0, token_type::bang, "!">, literal_expr<123.52>>>>);
static_assert(test<
    "-false;",
    expression_stmt<unary_expr<token_ct<0, token_type::minus, "-">, literal_expr<false>>>>);
static_assert(test<
    "!!true;",
    expression_stmt<unary_expr<token_ct<0, token_type::bang, "!">, unary_expr<token_ct<1, token_type::bang, "!">, literal_expr<true>>>>>);
static_assert(test<
    "2 * 3;",
    expression_stmt<
        binary_expr<
            literal_expr<2.0>,
            token_ct<2, token_type::star, "*">,
            literal_expr<3.0>>>>);
static_assert(test<
    "1 * -2 / !false;",
    expression_stmt<
        binary_expr<
            binary_expr<
                literal_expr<1.0>,
                token_ct<2, token_type::star, "*">,
                unary_expr<token_ct<4, token_type::minus, "-">, literal_expr<2.0>>>,
            token_ct<7, token_type::slash, "/">,
            unary_expr<token_ct<9, token_type::bang, "!">, literal_expr<false>>>>>);
static_assert(test<
    R"(13.3 + "stuff";)",
    expression_stmt<
        binary_expr<
            literal_expr<13.3>,
            token_ct<5, token_type::plus, "+">,
            literal_expr<"stuff"_ct>>>>);
static_assert(test<
    R"(nil - nil - nil;)",
    expression_stmt<
        binary_expr<
            binary_expr<
                literal_expr<nil>,
                token_ct<4, token_type::minus, "-">,
                literal_expr<nil>>,
            token_ct<10, token_type::minus, "-">,
            literal_expr<nil>>>>);
static_assert(test<
    "1 + 2 * 3 - 4;",
    expression_stmt<
        binary_expr<
            binary_expr<
                literal_expr<1.0>,
                token_ct<2, token_type::plus, "+">,
                binary_expr<
                    literal_expr<2.0>,
                    token_ct<6, token_type::star, "*">,
                    literal_expr<3.0>>>,
            token_ct<10, token_type::minus, "-">,
            literal_expr<4.0>>>>);
static_assert(test<"1 * 2 + 3 / 4;",
    expression_stmt<
        binary_expr<
            binary_expr<
                literal_expr<1.0>,
                token_ct<2, token_type::star, "*">,
                literal_expr<2.0>>,
            token_ct<6, token_type::plus, "+">,
            binary_expr<
                literal_expr<3.0>,
                token_ct<10, token_type::slash, "/">,
                literal_expr<4.0>>>>>);
static_assert(test<"true > nil;",
    expression_stmt<
        binary_expr<literal_expr<true>, token_ct<5, token_type::greater, ">">, literal_expr<nil>>>>);
static_assert(test<"1 > 2 <= 3 >= 0;",
    expression_stmt<
        binary_expr<
            binary_expr<
                binary_expr<
                    literal_expr<1.0>,
                    token_ct<2, token_type::greater, ">">,
                    literal_expr<2.0>>,
                token_ct<6, token_type::less_equal, "<=">,
                literal_expr<3.0>>,
            token_ct<11, token_type::greater_equal, ">=">,
            literal_expr<0.0>>>>);
static_assert(test<"nil != nil == false;",
    expression_stmt<
        binary_expr<
            binary_expr<
                literal_expr<nil>,
                token_ct<4, token_type::bang_equal, "!=">,
                literal_expr<nil>>,
            token_ct<11, token_type::equal_equal, "==">,
            literal_expr<false>>>>);
static_assert(test<"7 == 2 * 3.5;",
    expression_stmt<
        binary_expr<
            literal_expr<7.0>,
            token_ct<2, token_type::equal_equal, "==">,
            binary_expr<
                literal_expr<2.0>,
                token_ct<7, token_type::star, "*">,
                literal_expr<3.5>>>>>);
static_assert(test<"(7 == 2) * 3.5;"_ct,
    expression_stmt<
        binary_expr<
            grouping_expr<
                binary_expr<
                    literal_expr<7.0>,
                    token_ct<3, token_type::equal_equal, "==">,
                    literal_expr<2.0>>>,
            token_ct<9, token_type::star, "*">,
            literal_expr<3.5>>>>);
static_assert(test<R"(1; print ("foo");)",
    expression_stmt<literal_expr<1.0>>,
    print_stmt<grouping_expr<literal_expr<"foo"_ct>>>>);
static_assert(test < "var foo; var bar = foo;",
    var_stmt<
        token_ct<4, token_type::identifier, "foo">,
        void>,
    var_stmt<
        token_ct<13, token_type::identifier, "bar">,
        variable_expr<token_ct<19, token_type::identifier, "foo">>>>);

}