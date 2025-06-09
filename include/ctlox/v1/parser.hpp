#pragma once

#include <ctlox/v1/ct.hpp>
#include <ctlox/v1/types.hpp>

namespace ctlox::v1 {
struct stmts {
    template <typename... Statements>
    struct stmt_list_t {
        struct given_statements {
            using has_f0 = void;
            template <accepts_pack C>
            using f0 = calln<C, Statements...>;
        };
    };

    struct end_of_list { };
    struct block_separator { };
    using new_stmt_list = stmt_list_t<end_of_list>;

    struct pop_block_impl {
    private:
        template <typename List, typename Statement>
        using list_push = run<given_one<List>, from_list, prepend<Statement>, listed>;
        template <typename List>
        using to_block_stmt = run<given_one<List>, from_list, applied<block_stmt>, returned>;

        template <bool at_block_separator = false>
        struct impl {
            template <typename C, typename List, typename Statement, typename T, typename... Ts>
            using f = impl<std::is_same_v<T, block_separator>>::template f<C, list_push<List, Statement>, T, Ts...>;
        };

        template <>
        struct impl<true> {
            template <typename C, typename List, typename EndOfBlock, typename... Ts>
            using f = calln<C, to_block_stmt<List>, Ts...>;
        };

    public:
        using has_fn = void;
        template <accepts_pack C, typename T, typename... Ts>
        using fn = impl<std::is_same_v<T, block_separator>>::template f<C, list<>, T, Ts...>;
    };

    struct list_stmts_impl {
    private:
        template <typename List, typename Statement>
        using list_push = run<given_one<List>, from_list, prepend<Statement>, listed>;

        template <bool at_end_of_list = false>
        struct impl {
            template <typename C, typename List, typename Statement, typename T, typename... Ts>
            using f = impl<std::is_same_v<T, end_of_list>>::template f<C, list_push<List, Statement>, T, Ts...>;
        };

        template <>
        struct impl<true> {
            template <typename C, typename List, typename EndOfList>
            using f = call1<compose<from_list, C>, List>;
        };

    public:
        using has_fn = void;
        template <accepts_pack C, typename T, typename... Ts>
        using fn = impl<std::is_same_v<T, end_of_list>>::template f<C, list<>, T, Ts...>;
    };

    template <typename StmtList, typename Statement>
    using push_statement = run<
        typename StmtList::given_statements,
        prepend<Statement>,
        applied<stmt_list_t>,
        returned>;

    template <typename StmtList>
    using push_block = run<
        typename StmtList::given_statements,
        prepend<block_separator>,
        applied<stmt_list_t>,
        returned>;

    template <typename StmtList>
    using pop_block = run<
        typename StmtList::given_statements,
        pop_block_impl,
        applied<stmt_list_t>,
        returned>;

    template <typename StmtList>
    using list_stmts = compose<
        typename StmtList::given_statements,
        list_stmts_impl>;
};

struct parse_ct {
private:
    struct consume_expression_workaround;
    template <std::size_t N>
    using consume_expression_t = typename dcall<consume_expression_workaround, N>::type;

    struct consume_right_paren {
        template <bool is_right_paren>
        struct impl {
            static_assert(false, "Expect ')' after expression.");
        };

        template <>
        struct impl<true> {
            template <typename C, typename StmtList, typename Expr, typename Token, typename... Ts>
            using f = calln<C, StmtList, grouping_expr<Expr>, Ts...>;
        };

        using has_fn = void;
        template <typename C, typename StmtList, typename Expr, typename Token, typename... Ts>
        using fn = impl<Token::type == token_type::right_paren>::template f<C, StmtList, Expr, Token, Ts...>;
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
            template <typename C, typename StmtList, typename Token, typename... Ts>
            using f = calln<C, StmtList, literal_expr<Token::literal>, Ts...>;
        };

        template <>
        struct impl<token_type::identifier> {
            template <typename C, typename StmtList, typename Token, typename... Ts>
            using f = calln<C, StmtList, variable_expr<Token>, Ts...>;
        };

        template <>
        struct impl<token_type::left_paren> {
            template <typename C, typename StmtList, typename Token, typename... Ts>
            using f = calln<
                compose<consume_expression_t<sizeof...(Ts)>, consume_right_paren, C>,
                StmtList, Ts...>;
        };

        using has_fn = void;
        template <typename C, typename StmtList, typename Token, typename... Ts>
        using fn = impl<Token::type>::template f<C, StmtList, Token, Ts...>;
    };

    template <typename Token>
    struct bind_unary_expr {
        using has_fn = void;
        template <typename C, typename StmtList, typename Expr, typename... Ts>
        using fn = calln<C, StmtList, unary_expr<Token, Expr>, Ts...>;
    };

    struct consume_unary {
        template <token_type type>
        struct impl : consume_primary::impl<type> { };

        template <token_type type>
            requires(type == token_type::bang || type == token_type::minus)
        struct impl<type> {
            template <typename C, typename StmtList, typename Token, typename... Ts>
            using f = calln<
                compose<consume_unary, bind_unary_expr<Token>, C>,
                StmtList, Ts...>;
        };

        using has_fn = void;
        template <typename C, typename StmtList, typename Token, typename... Ts>
        using fn = impl<Token::type>::template f<C, StmtList, Token, Ts...>;
    };

    template <typename LeftExpr, typename Token>
    struct bind_binary_expr {
        using has_fn = void;
        template <typename C, typename StmtList, typename RightExpr, typename... Ts>
        using fn = calln<C, StmtList, binary_expr<LeftExpr, Token, RightExpr>, Ts...>;
    };

    struct maybe_match_product_op {
        template <bool is_product_op>
        struct impl {
            template <typename C, typename... Ts>
            using f = calln<C, Ts...>;
        };

        template <>
        struct impl<true> {
            template <typename C, typename StmtList, typename Expr, typename Token, typename... Ts>
            using f = calln<
                compose<consume_unary, bind_binary_expr<Expr, Token>, maybe_match_product_op, C>,
                StmtList, Ts...>;
        };

        static constexpr bool is_product_op(token_type type)
        {
            return type == token_type::slash || type == token_type::star;
        }

        using has_fn = void;
        template <typename C, typename StmtList, typename Expr, typename Token, typename... Ts>
        using fn = impl<is_product_op(Token::type)>::template f<C, StmtList, Expr, Token, Ts...>;
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
            template <typename C, typename StmtList, typename Expr, typename Token, typename... Ts>
            using f = calln<
                compose<consume_factor, bind_binary_expr<Expr, Token>, maybe_match_sum_op, C>,
                StmtList, Ts...>;
        };

        static constexpr bool is_sum_op(token_type type)
        {
            return type == token_type::plus || type == token_type::minus;
        }

        using has_fn = void;
        template <typename C, typename StmtList, typename Expr, typename Token, typename... Ts>
        using fn = impl<is_sum_op(Token::type)>::template f<C, StmtList, Expr, Token, Ts...>;
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
            template <typename C, typename StmtList, typename Expr, typename Token, typename... Ts>
            using f = calln<
                compose<consume_term, bind_binary_expr<Expr, Token>, maybe_match_comparison_op, C>,
                StmtList, Ts...>;
        };

        static constexpr bool is_comparison_op(token_type type)
        {
            return type == token_type::greater
                || type == token_type::greater_equal
                || type == token_type::less
                || type == token_type::less_equal;
        }

        using has_fn = void;
        template <typename C, typename StmtList, typename Expr, typename Token, typename... Ts>
        using fn = impl<is_comparison_op(Token::type)>::template f<C, StmtList, Expr, Token, Ts...>;
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
            template <typename C, typename StmtList, typename Expr, typename Token, typename... Ts>
            using f = calln<
                compose<consume_comparison, bind_binary_expr<Expr, Token>, maybe_match_equality_op, C>,
                StmtList, Ts...>;
        };

        static constexpr bool is_equality_op(token_type type)
        {
            return type == token_type::equal_equal || type == token_type::bang_equal;
        }

        using has_fn = void;
        template <typename C, typename StmtList, typename Expr, typename Token, typename... Ts>
        using fn = impl<is_equality_op(Token::type)>::template f<C, StmtList, Expr, Token, Ts...>;
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
        template <typename C, typename StmtList, typename ValueExpr, typename... Ts>
        using fn = calln<C, StmtList, assign_expr<Name, ValueExpr>, Ts...>;
    };

    struct maybe_match_assign_op {
        template <bool is_assign_op>
        struct impl {
            template <typename C, typename... Ts>
            using f = calln<C, Ts...>;
        };

        template <>
        struct impl<true> {
            template <typename C, typename StmtList, typename Expr, typename Token, typename... Ts>
            using f = calln<
                compose<consume_assignment, bind_assign_expr<Expr>, C>,
                StmtList, Ts...>;
        };

        static constexpr bool is_assign_op(token_type type) { return type == token_type::equal; }

        using has_fn = void;
        template <typename C, typename StmtList, typename Expr, typename Token, typename... Ts>
        using fn = impl<is_assign_op(Token::type)>::template f<C, StmtList, Expr, Token, Ts...>;
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
            template <typename C, typename StmtList, typename Expr, typename Token, typename... Ts>
            using f = calln<C, StmtList, Expr, Ts...>;
        };

        using has_fn = void;
        template <typename C, typename StmtList, typename Expr, typename Token, typename... Ts>
        using fn = impl<(Token::type == token_type::semicolon)>::template f<C, StmtList, Expr, Token, Ts...>;
    };

    struct consume_declaration;

    template <template <typename...> typename Stmt, typename... Args>
    struct to_statement {
        using has_fn = void;
        template <typename C, typename StmtList, typename Expr, typename... Ts>
        using fn = calln<C, stmts::push_statement<StmtList, Stmt<Args..., Expr>>, Ts...>;
    };

    struct maybe_match_end_of_block {
        template <bool is_right_brace = false>
        struct impl {
            template <typename C, typename StmtList, typename... Ts>
            using f = calln<
                compose<consume_declaration, maybe_match_end_of_block, C>,
                StmtList, Ts...>;
        };

        template <>
        struct impl<true> {
            template <typename C, typename StmtList, typename RightBrace, typename... Ts>
            using f = calln<C, stmts::pop_block<StmtList>, Ts...>;
        };

        using has_fn = void;
        template <accepts_pack C, typename StmtList, typename Token, typename... Ts>
        using fn = impl<(Token::type == token_type::right_brace)>::template f<C, StmtList, Token, Ts...>;
    };

    struct consume_block {
        using has_fn = void;
        template <accepts_pack C, typename StmtList, typename Token, typename... Ts>
        using fn = maybe_match_end_of_block::fn<C, stmts::push_block<StmtList>, Token, Ts...>;
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
            template <typename C, typename StmtList, typename... Ts>
            using f = calln<
                compose<consume_expression, match_semicolon, to_statement<expression_stmt>, C>,
                StmtList, Ts...>;
        };

        template <>
        struct impl<strategy::print> {
            template <typename C, typename StmtList, typename Token, typename... Ts>
            using f = calln<
                compose<consume_expression, match_semicolon, to_statement<print_stmt>, C>,
                StmtList, Ts...>;
        };

        template <>
        struct impl<strategy::block> {
            template <typename C, typename StmtList, typename Token, typename... Ts>
            using f = calln<compose<consume_block, C>, StmtList, Ts...>;
        };

        using has_fn = void;
        template <accepts_pack C, typename StmtList, typename Token, typename... Ts>
        using fn = impl<classify(Token::type)>::template f<C, StmtList, Token, Ts...>;
    };

    struct maybe_match_variable_init {
        template <bool has_init>
        struct impl {
            template <typename C, typename StmtList, typename... Ts>
            using f = calln<C, StmtList, literal_expr<nil>, Ts...>;
        };

        template <>
        struct impl<true> {
            template <typename C, typename StmtList, typename Equal, typename... Ts>
            using f = calln<compose<consume_expression, C>, StmtList, Ts...>;
        };

        using has_fn = void;
        template <typename C, typename StmtList, typename Token, typename... Ts>
        using fn = impl<(Token::type == token_type::equal)>::template f<C, StmtList, Token, Ts...>;
    };

    struct consume_variable_declaration {
        template <bool has_id>
        struct impl {
            static_assert(false, "Expect variable name.");
        };

        template <>
        struct impl<true> {
            template <typename C, typename StmtList, typename Var, typename Identifier, typename... Ts>
            using f = calln<
                compose<maybe_match_variable_init, match_semicolon, to_statement<var_stmt, Identifier>, C>,
                StmtList, Ts...>;
        };

        using has_fn = void;
        template <typename C, typename StmtList, typename Var, typename Token, typename... Ts>
        using fn = impl<(Token::type == token_type::identifier)>::template f<C, StmtList, Var, Token, Ts...>;
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
        template <accepts_pack C, typename StmtList, typename Token, typename... Ts>
        using fn = impl<classify(Token::type)>::template fn<C, StmtList, Token, Ts...>;
    };

    struct consume_program {
        template <bool is_eof>
        struct impl {
            template <typename C, typename StmtList, typename... Ts>
            using f = calln<compose<consume_declaration, consume_program, C>, StmtList, Ts...>;
        };

        template <>
        struct impl<true> {
            template <typename C, typename StmtList, typename Eof>
            using f = initiate<compose<stmts::list_stmts<StmtList>, C>>;
        };

        using has_fn = void;
        template <accepts_pack C, typename StmtList, typename Token, typename... Ts>
        using fn = impl<(Token::type == token_type::eof)>::template f<C, StmtList, Token, Ts...>;
    };

public:
    using has_fn = void;
    template <accepts_pack C, typename... Tokens>
    using fn = calln<compose<consume_program, C>, stmts::new_stmt_list, Tokens...>;
};
}
