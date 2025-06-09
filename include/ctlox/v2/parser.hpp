#pragma once

#include <ctlox/v2/exception.hpp>
#include <ctlox/v2/expression.hpp>
#include <ctlox/v2/statement.hpp>
#include <ctlox/v2/token.hpp>

#include <span>
#include <vector>

namespace ctlox::v2 {

class parser {
public:
    constexpr explicit parser(std::span<const token_t> tokens)
        : tokens_(tokens) { }

    constexpr std::vector<stmt_ptr> parse() && {
        std::vector<stmt_ptr> statements;
        while (!at_end()) {
            statements.push_back(declaration());
        }

        return statements;
    }

private:
    template <std::same_as<token_type>... TokenTypes>
    [[nodiscard]] constexpr bool match(TokenTypes... types) {
        const bool matches = (check(types) || ...);
        if (matches)
            advance();

        return matches;
    }

    constexpr expr_ptr expression() { return assignment(); }

    constexpr stmt_ptr declaration() {
        if (match(token_type::_var))
            return var_declaration();

        return statement();
    }

    constexpr stmt_ptr statement() {
        if (match(token_type::_break))
            return break_statement();
        if (match(token_type::_for))
            return for_statement();
        if (match(token_type::_if))
            return if_statement();
        if (match(token_type::_print))
            return print_statement();
        if (match(token_type::_while))
            return while_statement();
        if (match(token_type::left_brace))
            return make_stmt(block_stmt { .statements_ = block() });

        return expression_statement();
    }

    constexpr stmt_ptr break_statement() {
        if (loop_depth_ == 0) {
            throw parse_error(previous(), "'break' may only appear within a loop.");
        }

        const auto& keyword = previous();

        consume(token_type::semicolon, "Expect ';' after 'break'.");
        return make_stmt(break_stmt { .keyword_ = keyword });
    }

    constexpr stmt_ptr for_statement() {
        consume(token_type::left_paren, "Expect '(' after 'for'.");

        stmt_ptr initializer;
        if (match(token_type::semicolon)) {
        } else if (match(token_type::_var)) {
            initializer = var_declaration();
        } else {
            initializer = expression_statement();
        }

        expr_ptr condition;
        if (!check(token_type::semicolon)) {
            condition = expression();
        }
        consume(token_type::semicolon, "Expect ';' after loop condition.");

        expr_ptr increment;
        if (!check(token_type::right_paren)) {
            increment = expression();
        }
        consume(token_type::right_paren, "Expect ')' after for clauses.");

        ++loop_depth_;
        stmt_ptr body = statement();
        --loop_depth_;

        if (increment) {
            stmt_ptr increment_stmt = make_stmt(expression_stmt { .expression_ = std::move(increment) });

            stmt_list block;
            block.push_back(std::move(body));
            block.push_back(std::move(increment_stmt));
            body = make_stmt(block_stmt { .statements_ = std::move(block) });
        }

        if (!condition) {
            condition = make_expr(literal_expr { .value_ = true });
        }
        body = make_stmt(while_stmt { .condition_ = std::move(condition), .body_ = std::move(body) });

        if (initializer) {
            stmt_list block;
            block.push_back(std::move(initializer));
            block.push_back(std::move(body));
            body = make_stmt(block_stmt { .statements_ = std::move(block) });
        }

        return body;
    }

    constexpr stmt_ptr if_statement() {
        consume(token_type::left_paren, "Expect '(' after 'if'.");
        expr_ptr condition = expression();
        consume(token_type::right_paren, "Expect ')' after condition.");

        stmt_ptr then_branch = statement();
        stmt_ptr else_branch;
        if (match(token_type::_else)) {
            else_branch = statement();
        }

        return make_stmt(
            if_stmt {
                .condition_ = std::move(condition),
                .then_branch_ = std::move(then_branch),
                .else_branch_ = std::move(else_branch),
            });
    }

    constexpr stmt_ptr print_statement() {
        const token_t& keyword = previous();
        expr_ptr expr = expression();
        consume(token_type::semicolon, "Expect ';' after value.");

        const token_t synthetic_callee_name {
            .type_ = token_type::identifier,
            .lexeme_ = "println",
            .literal_ = none,
            .line_ = keyword.line_,
        };
        expr_ptr callee = make_expr(variable_expr { .name_ = synthetic_callee_name });

        expr_list arguments;
        arguments.push_back(std::move(expr));

        expr_ptr call = make_expr(
            call_expr {
                .paren_ = keyword,
                .callee_ = std::move(callee),
                .arguments_ = std::move(arguments),
            });

        return make_stmt(expression_stmt { .expression_ = std::move(call) });
    }

    constexpr stmt_ptr var_declaration() {
        token_t name = consume(token_type::identifier, "Expect variable name.");

        expr_ptr initializer;
        if (match(token_type::equal)) {
            initializer = expression();
        }

        consume(token_type::semicolon, "Expect ';' after variable declaration.");
        return make_stmt(var_stmt { .name_ = name, .initializer_ = std::move(initializer) });
    }

    constexpr stmt_ptr while_statement() {
        consume(token_type::left_paren, "Expect ')' after 'while'.");
        expr_ptr condition = expression();
        consume(token_type::right_paren, "Expect ')' after condition.");

        ++loop_depth_;
        stmt_ptr body = statement();
        --loop_depth_;

        return make_stmt(
            while_stmt {
                .condition_ = std::move(condition),
                .body_ = std::move(body),
            });
    }

    constexpr stmt_ptr expression_statement() {
        expr_ptr expr = expression();
        consume(token_type::semicolon, "Expect ';' after expression.");
        return make_stmt(expression_stmt { .expression_ = std::move(expr) });
    }

    constexpr std::vector<stmt_ptr> block() {
        std::vector<stmt_ptr> statements;

        while (!check(token_type::right_brace) && !at_end()) {
            statements.push_back(declaration());
        }

        consume(token_type::right_brace, "Expect '}' after block.");
        return statements;
    }

    constexpr expr_ptr assignment() {
        expr_ptr expr = _or();

        if (match(token_type::equal)) {
            token_t equals = previous();
            expr_ptr value = assignment();

            if (const auto* var_expr = expr->get_if<variable_expr>()) {
                token_t name = var_expr->name_;
                expr = make_expr(assign_expr { .name_ = name, .value_ = std::move(value) });
            } else {
                throw parse_error(equals, "Invalid assignment target.");
            }
        }

        return expr;
    }

    constexpr expr_ptr _or() {
        expr_ptr expr = _and();

        while (match(token_type::_or)) {
            token_t oper = previous();
            expr_ptr right = _and();

            expr = make_expr(
                logical_expr {
                    .operator_ = oper,
                    .left_ = std::move(expr),
                    .right_ = std::move(right),
                });
        }

        return expr;
    }

    constexpr expr_ptr _and() {
        expr_ptr expr = equality();

        while (match(token_type::_and)) {
            token_t oper = previous();
            expr_ptr right = equality();

            expr = make_expr(
                logical_expr {
                    .operator_ = oper,
                    .left_ = std::move(expr),
                    .right_ = std::move(right),
                });
        }

        return expr;
    }

    constexpr expr_ptr equality() {
        expr_ptr expr = comparison();

        while (match(token_type::bang_equal, token_type::equal_equal)) {
            token_t oper = previous();
            expr_ptr right = comparison();

            expr = make_expr(
                binary_expr {
                    .operator_ = oper,
                    .left_ = std::move(expr),
                    .right_ = std::move(right),
                });
        }

        return expr;
    }

    constexpr expr_ptr comparison() {
        expr_ptr expr = term();

        while (match(token_type::greater, token_type::greater_equal, token_type::less, token_type::less_equal)) {
            token_t oper = previous();
            expr_ptr right = term();

            expr = make_expr(
                binary_expr {
                    .operator_ = oper,
                    .left_ = std::move(expr),
                    .right_ = std::move(right),
                });
        }

        return expr;
    }

    constexpr expr_ptr term() {
        expr_ptr expr = factor();

        while (match(token_type::minus, token_type::plus)) {
            token_t oper = previous();
            expr_ptr right = factor();

            expr = make_expr(
                binary_expr {
                    .operator_ = oper,
                    .left_ = std::move(expr),
                    .right_ = std::move(right),
                });
        }

        return expr;
    }

    constexpr expr_ptr factor() {
        expr_ptr expr = unary();

        while (match(token_type::slash, token_type::star)) {
            token_t oper = previous();
            expr_ptr right = unary();

            expr = make_expr(
                binary_expr {
                    .operator_ = oper,
                    .left_ = std::move(expr),
                    .right_ = std::move(right),
                });
        }

        return expr;
    }

    constexpr expr_ptr unary() {
        expr_ptr expr;

        if (match(token_type::bang, token_type::minus)) {
            token_t oper = previous();
            expr_ptr right = unary();

            expr = make_expr(unary_expr { .operator_ = oper, .right_ = std::move(right) });
        } else {
            expr = call();
        }

        return expr;
    }

    constexpr expr_ptr finish_call(expr_ptr callee) {
        expr_list arguments;
        if (!check(token_type::right_paren)) {
            do {
                if (arguments.size() >= 255) {
                    throw parse_error(peek(), "Can't have more than 255 arguments.");
                }

                arguments.push_back(expression());
            } while (match(token_type::comma));
        }

        token_t paren = consume(token_type::right_paren, "Expect ')' after arguments.");

        return make_expr(
            call_expr {
                .paren_ = paren,
                .callee_ = std::move(callee),
                .arguments_ = std::move(arguments),
            });
    }

    constexpr expr_ptr call() {
        expr_ptr expr = primary();

        while (true) {
            if (match(token_type::left_paren)) {
                expr = finish_call(std::move(expr));
            } else {
                break;
            }
        }

        return expr;
    }

    constexpr expr_ptr primary() {
        if (match(token_type::_false, token_type::_true, token_type::_nil, token_type::number, token_type::string)) {
            return make_expr(literal_expr { .value_ = previous().literal_ });
        }

        if (match(token_type::identifier)) {
            return make_expr(variable_expr { .name_ = previous() });
        }

        if (match(token_type::left_paren)) {
            expr_ptr expr = expression();
            consume(token_type::right_paren, "Expect ')' after expression.");

            return make_expr(grouping_expr { .expr_ = std::move(expr) });
        }

        throw parse_error(peek(), "Expect expression.");
    }

    constexpr token_t consume(token_type type, const char* message) {
        if (check(type))
            return advance();

        throw parse_error(peek(), message);
    }

    [[nodiscard]] constexpr bool check(token_type type) const {
        if (at_end())
            return false;

        return peek().type_ == type;
    }

    constexpr token_t advance() {
        if (!at_end())
            ++current_;

        return previous();
    }

    [[nodiscard]] constexpr bool at_end() const { return peek().type_ == token_type::eof; }

    [[nodiscard]] constexpr const token_t& peek() const { return tokens_[current_]; }

    [[nodiscard]] constexpr const token_t& previous() const { return tokens_[current_ - 1]; }

    std::span<const token_t> tokens_;
    int current_ = 0;

    int loop_depth_ = 0;
};

constexpr std::vector<stmt_ptr> parse(std::span<const token_t> tokens) { return parser(tokens).parse(); }

}  // namespace ctlox::v2