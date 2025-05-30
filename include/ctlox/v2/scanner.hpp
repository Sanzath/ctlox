#pragma once

#include <ctlox/common/characters.hpp>
#include <ctlox/common/numbers.hpp>
#include <ctlox/v2/exception.hpp>
#include <ctlox/v2/token.hpp>

#include <vector>

namespace ctlox::v2 {

class scanner {
public:
    constexpr explicit scanner(std::string_view source)
        : source_(source) { }

    constexpr std::vector<token_t> scan_tokens() && {
        while (!at_end()) {
            start_ = current_;
            scan_token();
        }

        tokens_.emplace_back(token_type::eof, "", none, line_);

        return std::move(tokens_);
    }

private:
    constexpr void scan_token() {
        char c = advance();
        switch (c) {
        case '(':
            add_token(token_type::left_paren);
            break;
        case ')':
            add_token(token_type::right_paren);
            break;
        case '{':
            add_token(token_type::left_brace);
            break;
        case '}':
            add_token(token_type::right_brace);
            break;
        case ',':
            add_token(token_type::comma);
            break;
        case '.':
            add_token(token_type::dot);
            break;
        case '-':
            add_token(token_type::minus);
            break;
        case '+':
            add_token(token_type::plus);
            break;
        case ';':
            add_token(token_type::semicolon);
            break;
        case '*':
            add_token(token_type::star);
            break;
        case '!':
            add_token(match('=') ? token_type::bang_equal : token_type::bang);
            break;
        case '=':
            add_token(match('=') ? token_type::equal_equal : token_type::equal);
            break;
        case '<':
            add_token(match('=') ? token_type::less_equal : token_type::less);
            break;
        case '>':
            add_token(match('=') ? token_type::greater_equal : token_type::greater);
            break;
        case '/':
            if (match('/')) {
                while (peek() != '\n' && !at_end())
                    advance();
            } else {
                add_token(token_type::slash);
            }
            break;

        case ' ':
        case '\r':
        case '\t':
            // ignore whitespace
            break;

        case '\n':
            ++line_;
            break;

        case '"':
            string();
            break;

        default:
            if (is_digit(c)) {
                number();
            } else if (is_alpha(c)) {
                identifier();
            } else {
                // TODO: how to do error reporting?
                //       Canonical Lox reports to a global and continues.
                //       Just throwing here means only one scan error
                //       will be reported...
                throw scan_error(line_, "Invalid character");
            }
            break;
        }
    }

    constexpr void identifier() {
        while (is_alphanumeric(peek()))
            advance();

        auto text = source_.substr(start_, current_ - start_);
        auto type = identify_keyword(text);

        literal_t literal;
        switch (type) {
        case token_type::_true:
            literal = true;
            break;
        case token_type::_false:
            literal = false;
            break;
        case token_type::_nil:
            // WORKAROUND: MSVC thought that `literal = nil` was not a constant expression.
            literal = literal_t(nil);
            break;
        default:
            break;
        }
        add_token(type, literal);
    }

    constexpr void number() {
        while (is_digit(peek()))
            advance();

        if (peek() == '.' && is_digit(peek_next())) {
            // consume the '.'
            advance();

            while (is_digit(peek()))
                advance();
        }

        auto text = source_.substr(start_, current_ - start_);
        add_token(token_type::number, parse_double(text));
    }

    constexpr void string() {
        int start_line = line_;
        while (peek() != '"' && !at_end()) {
            if (peek() == '\n')
                ++line_;
            advance();
        }

        if (at_end()) {
            throw scan_error(start_line, "Unterminated string");
        }

        advance();

        // Trim the quotes
        auto value = source_.substr(start_ + 1, current_ - start_ - 2);
        add_token(token_type::string, value);
    }

    constexpr bool match(char expected) {
        if (at_end())
            return false;
        if (source_[current_] != expected)
            return false;

        ++current_;
        return true;
    }

    [[nodiscard]] constexpr char peek() const {
        if (at_end())
            return '\0';

        return source_[current_];
    }

    [[nodiscard]] constexpr char peek_next() const {
        if (current_ + 1 >= source_.size())
            return '\0';

        return source_[current_ + 1];
    }

    [[nodiscard]] constexpr bool at_end() const { return current_ >= source_.size(); }

    constexpr char advance() { return source_[current_++]; }

    constexpr void add_token(token_type type, literal_t literal = none) {
        auto text = source_.substr(start_, current_ - start_);
        tokens_.emplace_back(type, text, literal, line_);
    }

    std::string_view source_;

    std::vector<token_t> tokens_;

    int start_ = 0;
    int current_ = 0;
    int line_ = 1;
};

constexpr std::vector<token_t> scan(std::string_view source) { return scanner(source).scan_tokens(); }

}  // namespace ctlox::v2