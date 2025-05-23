#pragma once

#include "ct.h"

#include "types.h"

namespace ctlox {
struct parse_ct {
private:
    struct consume_expression_workaround;
    template <std::size_t N>
    using consume_expression_t = typename dcall<consume_expression_workaround, N>::type;

    struct consume_right_paren {
        template <bool is_right_paren>
        struct impl {
            // TODO: is there a way to error out without a hard error?
            // With tokens, we could just intersperse the errors in the tokens.
            // But that doesn't really work here.
            // -- Actually, by now parsing full statements, something could be done.
            static_assert(false, "Expect ')' after expression.");
        };

        template <>
        struct impl<true> {
            template <typename C, typename Expr, typename Token, typename... Ts>
            using f = calln<C, grouping_expr<Expr>, Ts...>;
        };

        using has_fn = void;
        template <typename C, typename Expr, typename Token, typename... Ts>
        using fn = impl<Token::type == token_type::right_paren>::template f<C, Expr, Token, Ts...>;
    };

    struct consume_primary {
        template <token_type type>
        struct impl {
            static_assert(false, "Expect expression.");
        };

        static constexpr bool is_literal(token_type type)
        {
            return type == token_type::string
                || type == token_type::number
                || type == token_type::_nil
                || type == token_type::_true
                || type == token_type::_false;
        }

        template <token_type type>
            requires(is_literal(type))
        struct impl<type> {
            template <typename C, typename Token, typename... Ts>
            using f = calln<C, literal_expr<Token::literal>, Ts...>;
        };

        template <>
        struct impl<token_type::identifier> {
            template <typename C, typename Token, typename... Ts>
            using f = calln<C, variable_expr<Token>, Ts...>;
        };

        template <>
        struct impl<token_type::left_paren> {
            template <typename C, typename Token, typename... Ts>
            using f = calln<
                compose<consume_expression_t<sizeof...(Ts)>, consume_right_paren, C>,
                Ts...>;
        };

        using has_fn = void;
        template <typename C, typename Token, typename... Ts>
        using fn = impl<Token::type>::template f<C, Token, Ts...>;
    };

    template <typename Token>
    struct bind_unary_expr {
        using has_fn = void;
        template <typename C, typename Expr, typename... Ts>
        using fn = calln<C, unary_expr<Token, Expr>, Ts...>;
    };

    struct consume_unary {
        template <token_type type>
        struct impl : consume_primary::impl<type> { };

        template <token_type type>
            requires(type == token_type::bang || type == token_type::minus)
        struct impl<type> {
            template <typename C, typename Token, typename... Ts>
            using f = calln<
                compose<consume_unary, bind_unary_expr<Token>, C>,
                Ts...>;
        };

        using has_fn = void;
        template <typename C, typename Token, typename... Ts>
        using fn = impl<Token::type>::template f<C, Token, Ts...>;
    };

    template <typename LeftExpr, typename Token>
    struct bind_binary_expr {
        using has_fn = void;
        template <typename C, typename RightExpr, typename... Ts>
        using fn = calln<C, binary_expr<LeftExpr, Token, RightExpr>, Ts...>;
    };

    struct maybe_match_product_op {
        template <bool is_product_op>
        struct impl {
            template <typename C, typename... Ts>
            using f = calln<C, Ts...>;
        };

        template <>
        struct impl<true> {
            template <typename C, typename Expr, typename Token, typename... Ts>
            using f = calln<
                compose<consume_unary, bind_binary_expr<Expr, Token>, maybe_match_product_op, C>,
                Ts...>;
        };

        static constexpr bool is_product_op(token_type type)
        {
            return type == token_type::slash || type == token_type::star;
        }

        using has_fn = void;
        template <typename C, typename Expr, typename Token, typename... Ts>
        using fn = impl<is_product_op(Token::type)>::template f<C, Expr, Token, Ts...>;
    };

    using consume_factor = compose<consume_unary, maybe_match_product_op>;

    struct maybe_match_sum_op {
        template <bool is_sum_op>
        struct impl {
            template <typename C, typename... Ts>
            using f = calln<C, Ts...>;
        };

        template <>
        struct impl<true> {
            template <typename C, typename Expr, typename Token, typename... Ts>
            using f = calln<
                compose<consume_factor, bind_binary_expr<Expr, Token>, maybe_match_sum_op, C>,
                Ts...>;
        };

        static constexpr bool is_sum_op(token_type type)
        {
            return type == token_type::plus || type == token_type::minus;
        }

        using has_fn = void;
        template <typename C, typename Expr, typename Token, typename... Ts>
        using fn = impl<is_sum_op(Token::type)>::template f<C, Expr, Token, Ts...>;
    };

    using consume_term = compose<consume_factor, maybe_match_sum_op>;

    struct maybe_match_comparison_op {
        template <bool is_comparison_op>
        struct impl {
            template <typename C, typename... Ts>
            using f = calln<C, Ts...>;
        };

        template <>
        struct impl<true> {
            template <typename C, typename Expr, typename Token, typename... Ts>
            using f = calln<
                compose<consume_term, bind_binary_expr<Expr, Token>, maybe_match_comparison_op, C>,
                Ts...>;
        };

        static constexpr bool is_comparison_op(token_type type)
        {
            return type == token_type::greater
                || type == token_type::greater_equal
                || type == token_type::less
                || type == token_type::less_equal;
        }

        using has_fn = void;
        template <typename C, typename Expr, typename Token, typename... Ts>
        using fn = impl<is_comparison_op(Token::type)>::template f<C, Expr, Token, Ts...>;
    };

    using consume_comparison = compose<consume_term, maybe_match_comparison_op>;

    struct maybe_match_equality_op {
        template <bool is_equality_op>
        struct impl {
            template <typename C, typename... Ts>
            using f = calln<C, Ts...>;
        };

        template <>
        struct impl<true> {
            template <typename C, typename Expr, typename Token, typename... Ts>
            using f = calln<
                compose<consume_comparison, bind_binary_expr<Expr, Token>, maybe_match_equality_op, C>,
                Ts...>;
        };

        static constexpr bool is_equality_op(token_type type)
        {
            return type == token_type::equal_equal || type == token_type::bang_equal;
        }

        using has_fn = void;
        template <typename C, typename Expr, typename Token, typename... Ts>
        using fn = impl<is_equality_op(Token::type)>::template f<C, Expr, Token, Ts...>;
    };

    using consume_equality = compose<consume_comparison, maybe_match_equality_op>;

    struct maybe_match_assign_op;

    using consume_assignment = compose<consume_equality, maybe_match_assign_op>;

    template <typename LeftExpr>
    struct bind_assign_expr {
        static_assert(false, "Invalid assignment target.");
    };

    template <typename Name>
    struct bind_assign_expr<variable_expr<Name>> {
        using has_fn = void;
        template <typename C, typename ValueExpr, typename... Ts>
        using fn = calln<C, assign_expr<Name, ValueExpr>, Ts...>;
    };

    struct maybe_match_assign_op {
        template <bool is_assign_op>
        struct impl {
            template <typename C, typename... Ts>
            using f = calln<C, Ts...>;
        };

        template <>
        struct impl<true> {
            template <typename C, typename Expr, typename Token, typename... Ts>
            using f = calln<
                compose<consume_assignment, bind_assign_expr<Expr>, C>,
                Ts...>;
        };

        static constexpr bool is_assign_op(token_type type) { return type == token_type::equal; }

        using has_fn = void;
        template <typename C, typename Expr, typename Token, typename... Ts>
        using fn = impl<is_assign_op(Token::type)>::template f<C, Expr, Token, Ts...>;
    };

    using consume_expression = consume_assignment;

    struct consume_expression_workaround {
        using type = consume_expression;
    };

    struct match_semicolon {
        template <bool is_semicolon>
        struct impl {
            static_assert(false, "Expect ';' after statement.");
        };

        template <>
        struct impl<true> {
            template <typename C, typename Expr, typename Token, typename... Ts>
            using f = calln<C, Expr, Ts...>;
        };

        using has_fn = void;
        template <typename C, typename Expr, typename Token, typename... Ts>
        using fn = impl<(Token::type == token_type::semicolon)>::template f<C, Expr, Token, Ts...>;
    };

    struct consume_declaration;

    template <template <typename...> typename Stmt, typename... Args>
    struct to_statement {
        using has_fn = void;
        template <typename C, typename Expr, typename... Ts>
        using fn = calln<C, Stmt<Args..., Expr>, Ts...>;
    };

    struct add_to_block {
        struct add_impl {
            template <typename BlockStmt>
            struct impl {
                static_assert(false, "logic error: not a block_stmt");
            };

            template <typename... Statements>
            struct impl<block_stmt<Statements...>> {
                template <typename C, typename Stmt, typename... Ts>
                using f = calln<C, block_stmt<Statements..., Stmt>, Ts...>;
            };

            using has_fn = void;
            template <accepts_pack C, typename BlockStmt, typename Stmt, typename... Ts>
            using fn = impl<BlockStmt>::template f<C, Stmt, Ts...>;
        };

        using has_fn = void;
        template <accepts_pack C, typename Stmt, typename... Ts>
        using fn = calln<
            compose<rotate<sizeof...(Ts)>, add_impl, rotate<1>, C>,
            Stmt, Ts...>;
    };

    struct maybe_match_end_of_block {
        template <bool is_right_brace = false>
        struct impl {
            template <typename C, typename... Ts>
            using f = calln<
                compose<consume_declaration, add_to_block, maybe_match_end_of_block, C>, Ts...>;
        };

        template <>
        struct impl<true> {
            template <typename C, typename RightBrace, typename... Ts>
            using f = calln<
                compose<rotate<sizeof...(Ts) - 1>, C>, Ts...>;
        };
        using has_fn = void;
        template <accepts_pack C, typename Token, typename... Ts>
        using fn = impl<(Token::type == token_type::right_brace)>::template f<C, Token, Ts...>;
    };

    struct consume_block {
        using has_fn = void;
        template <accepts_pack C, typename Token, typename... Ts>
        using fn = maybe_match_end_of_block::fn<C, Token, Ts..., block_stmt<>>;
    };

    struct consume_statement {
        enum class strategy {
            expression,
            print,
            block,
        };

        constexpr static strategy classify(token_type type)
        {
            switch (type) {
            case token_type::_print:
                return strategy::print;
            case token_type::left_brace:
                return strategy::block;
            default:
                return strategy::expression;
            }
        }

        template <strategy s>
        struct impl {
            static_assert(false, "logic error: unhandled case.");
        };

        template <>
        struct impl<strategy::expression> {
            template <typename C, typename... Ts>
            using f = calln<
                compose<consume_expression, match_semicolon, to_statement<expression_stmt>, C>,
                Ts...>;
        };

        template <>
        struct impl<strategy::print> {
            template <typename C, typename Token, typename... Ts>
            using f = calln<
                compose<consume_expression, match_semicolon, to_statement<print_stmt>, C>,
                Ts...>;
        };

        template <>
        struct impl<strategy::block> {
            template <typename C, typename Token, typename... Ts>
            using f = calln<compose<consume_block, C>, Ts...>;
        };

        using has_fn = void;
        template <accepts_pack C, typename Token, typename... Ts>
        using fn = impl<classify(Token::type)>::template f<C, Token, Ts...>;
    };

    struct maybe_match_variable_init {
        template <bool has_init>
        struct impl {
            template <typename C, typename... Ts>
            using f = calln<C, literal_expr<nil>, Ts...>;
        };

        template <>
        struct impl<true> {
            template <typename C, typename Equal, typename... Ts>
            using f = calln<compose<consume_expression, C>, Ts...>;
        };

        using has_fn = void;
        template <typename C, typename Token, typename... Ts>
        using fn = impl<(Token::type == token_type::equal)>::template f<C, Token, Ts...>;
    };

    struct consume_variable_declaration {
        template <bool has_id>
        struct impl {
            static_assert(false, "Expect variable name.");
        };

        template <>
        struct impl<true> {
            template <typename C, typename Var, typename Identifier, typename... Ts>
            using f = calln<
                compose<maybe_match_variable_init, match_semicolon, to_statement<var_stmt, Identifier>, C>,
                Ts...>;
        };

        using has_fn = void;
        template <typename C, typename Var, typename Token, typename... Ts>
        using fn = impl<(Token::type == token_type::identifier)>::template f<C, Var, Token, Ts...>;
    };

    struct consume_declaration {
        enum class strategy {
            statement,
            var,
        };

        static constexpr strategy classify(token_type type)
        {
            switch (type) {
            case token_type::_var:
                return strategy::var;
            default:
                return strategy::statement;
            }
        }

        template <strategy s>
        struct impl {
            static_assert(false, "logic error: unhandled case.");
        };

        template <>
        struct impl<strategy::statement> : consume_statement { };

        template <>
        struct impl<strategy::var> : consume_variable_declaration { };

        using has_fn = void;
        template <accepts_pack C, typename Token, typename... Ts>
        using fn = impl<classify(Token::type)>::template fn<C, Token, Ts...>;
    };

    struct consume_program {
        template <bool is_eof>
        struct impl {
            template <typename C, typename... Ts>
            using f = calln<compose<consume_declaration, rotate<1>, consume_program, C>, Ts...>;
        };

        template <>
        struct impl<true> {
            template <typename C, typename Eof, typename... Statements>
            using f = calln<C, Statements...>;
        };

        using has_fn = void;
        template <accepts_pack C, typename Token, typename... Ts>
        using fn = impl<(Token::type == token_type::eof)>::template f<C, Token, Ts...>;
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
static_assert(test<"var foo; var bar = foo;",
    var_stmt<
        token_ct<4, token_type::identifier, "foo">,
        literal_expr<nil>>,
    var_stmt<
        token_ct<13, token_type::identifier, "bar">,
        variable_expr<token_ct<19, token_type::identifier, "foo">>>>);
static_assert(test<
    "var foo; var bar; foo = (bar = 2) + 2;",
    var_stmt<
        token_ct<4, token_type::identifier, "foo">,
        literal_expr<nil>>,
    var_stmt<
        token_ct<13, token_type::identifier, "bar">,
        literal_expr<nil>>,
    expression_stmt<
        assign_expr<
            token_ct<18, token_type::identifier, "foo">,
            binary_expr<
                grouping_expr<
                    assign_expr<
                        token_ct<25, token_type::identifier, "bar">,
                        literal_expr<2.0>>>,
                token_ct<34, token_type::plus, "+">,
                literal_expr<2.0>>>>>);
static_assert(test<
    "var foo; { var bar = 1; foo = bar; } print foo;",
    var_stmt<
        token_ct<4, token_type::identifier, "foo">,
        literal_expr<nil>>,
    block_stmt<
        var_stmt<
            token_ct<15, token_type::identifier, "bar">,
            literal_expr<1.0>>,
        expression_stmt<
            assign_expr<
                token_ct<24, token_type::identifier, "foo">,
                variable_expr<token_ct<30, token_type::identifier, "bar">>>>>,
    print_stmt<variable_expr<token_ct<43, token_type::identifier, "foo">>>>);
}