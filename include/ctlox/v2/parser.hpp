#pragma once

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

    constexpr std::vector<stmt_ptr_t> parse() && {
        std::vector<stmt_ptr_t> statements;
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

    constexpr expr_ptr_t expression() { return assignment(); }

    constexpr stmt_ptr_t declaration() {
        if (match(token_type::_var))
            return var_declaration();

        return statement();
    }

    constexpr stmt_ptr_t statement() {
        if (match(token_type::_print))
            return print_statement();
        if (match(token_type::left_brace))
            return make_stmt(block_stmt { .statements_ = block() });

        return expression_statement();
    }

    constexpr stmt_ptr_t print_statement() {
        expr_ptr_t expr = expression();
        consume(token_type::semicolon, "Expect ',' after value.");
        return make_stmt(print_stmt { .expression_ = std::move(expr) });
    }

    constexpr stmt_ptr_t var_declaration() {
        token_t name = consume(token_type::identifier, "Expect variable name.");

        expr_ptr_t initializer;
        if (match(token_type::equal)) {
            initializer = expression();
        }

        consume(token_type::semicolon, "Expect ';' after variable declaration.");
        return make_stmt(var_stmt { .name_ = name, .initializer_ = std::move(initializer) });
    }

    constexpr stmt_ptr_t expression_statement() {
        expr_ptr_t expr = expression();
        consume(token_type::semicolon, "Expect ',' after expression.");
        return make_stmt(expression_stmt { .expression_ = std::move(expr) });
    }

    constexpr std::vector<stmt_ptr_t> block() {
        std::vector<stmt_ptr_t> statements;

        while (!check(token_type::right_brace) && !at_end()) {
            statements.push_back(declaration());
        }

        consume(token_type::right_brace, "Expect '}' after block.");
        return statements;
    }

    constexpr expr_ptr_t assignment() {
        expr_ptr_t expr = equality();

        if (match(token_type::equal)) {
            // token_t equals = previous();
            expr_ptr_t value = assignment();

            if (const auto* var_expr = expr->get_if<variable_expr>()) {
                token_t name = var_expr->name_;
                expr = make_expr(assign_expr { .name_ = name, .value_ = std::move(value) });
            } else {
                throw std::invalid_argument("Invalid assignment target.");
            }
        }

        return expr;
    }

    constexpr expr_ptr_t equality() {
        expr_ptr_t expr = comparison();

        while (match(token_type::bang_equal, token_type::equal_equal)) {
            token_t oper = previous();
            expr_ptr_t right = comparison();

            expr = make_expr(
                binary_expr {
                    .operator_ = oper,
                    .left_ = std::move(expr),
                    .right_ = std::move(right),
                });
        }

        return expr;
    }

    constexpr expr_ptr_t comparison() {
        expr_ptr_t expr = term();

        while (match(token_type::greater, token_type::greater_equal, token_type::less, token_type::less_equal)) {
            token_t oper = previous();
            expr_ptr_t right = term();

            expr = make_expr(
                binary_expr {
                    .operator_ = oper,
                    .left_ = std::move(expr),
                    .right_ = std::move(right),
                });
        }

        return expr;
    }

    constexpr expr_ptr_t term() {
        expr_ptr_t expr = factor();

        while (match(token_type::minus, token_type::plus)) {
            token_t oper = previous();
            expr_ptr_t right = factor();

            expr = make_expr(
                binary_expr {
                    .operator_ = oper,
                    .left_ = std::move(expr),
                    .right_ = std::move(right),
                });
        }

        return expr;
    }

    constexpr expr_ptr_t factor() {
        expr_ptr_t expr = unary();

        while (match(token_type::slash, token_type::star)) {
            token_t oper = previous();
            expr_ptr_t right = unary();

            expr = make_expr(
                binary_expr {
                    .operator_ = oper,
                    .left_ = std::move(expr),
                    .right_ = std::move(right),
                });
        }

        return expr;
    }

    constexpr expr_ptr_t unary() {
        expr_ptr_t expr;

        if (match(token_type::bang, token_type::minus)) {
            token_t oper = previous();
            expr_ptr_t right = unary();

            expr = make_expr(unary_expr { .operator_ = oper, .right_ = std::move(right) });
        } else {
            expr = primary();
        }

        return expr;
    }

    constexpr expr_ptr_t primary() {
        if (match(token_type::_false, token_type::_true, token_type::_nil, token_type::number, token_type::string)) {
            return make_expr(literal_expr { .value_ = previous().literal_ });
        }

        if (match(token_type::identifier)) {
            return make_expr(variable_expr { .name_ = previous() });
        }

        if (match(token_type::left_paren)) {
            expr_ptr_t expr = expression();
            consume(token_type::right_paren, "Expect ')' after expression.");

            return make_expr(grouping_expr { .expr_ = std::move(expr) });
        }

        throw std::invalid_argument("Expect expression.");
    }

    constexpr token_t consume(token_type type, std::string_view message) {
        if (check(type))
            return advance();

        throw std::invalid_argument(std::string(message));
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

    [[nodiscard]] constexpr token_t peek() const { return tokens_[current_]; }

    [[nodiscard]] constexpr token_t previous() const { return tokens_[current_ - 1]; }

    std::span<const token_t> tokens_;
    int current_ = 0;
};

constexpr std::vector<stmt_ptr_t> parse(std::span<const token_t> tokens) { return parser(tokens).parse(); }

}  // namespace ctlox::v2