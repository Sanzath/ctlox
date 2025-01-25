#pragma once

#include "token.h"
#include "error.h"

#include <vector>
#include <array>
#include <utility>
#include <string>

namespace ctlox {
    struct scan_output {
        std::vector<token> tokens_;
        std::vector<error> errors_;
    };

    class scanner {
    public:
        constexpr scanner(std::string_view source) : source_(source) {}

        constexpr scan_output scan_tokens() {
            while (!at_end()) {
                start_ = current_;
                scan_token_at();
            }

            tokens_.push_back({ .type_ = token_type::eof, .line_ = line_, });
            return { tokens_, errors_ };
        }

    private:
        constexpr void scan_token_at() {
            char c = advance();
            switch (c) {
            case '(': add_token(token_type::left_paren); break;
            case ')': add_token(token_type::right_paren); break;
            case '{': add_token(token_type::left_brace); break;
            case '}': add_token(token_type::right_brace); break;
            case ',': add_token(token_type::comma); break;
            case '.': add_token(token_type::dot); break;
            case '-': add_token(token_type::minus); break;
            case '+': add_token(token_type::plus); break;
            case ';': add_token(token_type::semicolon); break;
            case '*': add_token(token_type::star); break;
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
                    while (peek() != '\n' && !at_end()) advance();
                }
                else {
                    add_token(token_type::slash);
                }
                break;

            case ' ':
            case '\r':
            case '\t':
                break;

            case '\n':
                ++line_;
                break;

            case '"': string(); break;

            default:
                if (is_digit(c)) { number(); }
                else if (is_alpha(c)) { identifier(); }
                else {
                    errors_.push_back({ .line_ = line_, .message_ = "Unexpected character.", });
                }
                break;
            }
        }

        constexpr void identifier() {
            while (is_alphanumeric(peek())) advance();

            auto text = source_.substr(start_, current_);
            auto type = identifier_type(text);
            add_token(type);
        }

        static constexpr token_type identifier_type(std::string_view text) {

            constexpr auto keywords = std::to_array<std::pair<std::string_view, token_type>>({
                {"and", token_type::_and},
                {"class", token_type::_class},
                {"else", token_type::_else},
                {"false", token_type::_false},
                {"for", token_type::_for},
                {"fun", token_type::_fun},
                {"if", token_type::_if},
                {"nil", token_type::_nil},
                {"or", token_type::_or},
                {"print", token_type::_print},
                {"return", token_type::_return},
                {"super", token_type::_super},
                {"this", token_type::_this},
                {"true", token_type::_true},
                {"var", token_type::_var},
                {"while", token_type::_while},
                });

            const auto it = std::ranges::find(keywords, text, [](const auto& p) { return p.first; });
            if (it != keywords.end()) {
                return it->second;
            }
            return token_type::identifier;
        }

        constexpr void number() {
            while (is_digit(peek())) advance();

            if (peek() == '.' && is_digit(peek_next())) {
                advance();

                while (is_digit(peek())) advance();
            }

            auto text = std::string_view(source_).substr(start_, current_ - start_);
            add_token(token_type::number, parse_double(text));
        }

        constexpr void string() {
            while (peek() != '"' && !at_end()) {
                if (peek() == '\n') ++line_;
                advance();
            }

            if (at_end()) {
                errors_.push_back({ .line_ = line_, .message_ = "Unterminated string." });
                return;
            }

            advance();

            auto value = source_.substr(start_ + 1, current_ - start_ - 2);
            add_token(token_type::string, std::move(value));
        }

        constexpr bool match(char expected) {
            if (at_end()) return false;
            if (source_[current_] != expected) return false;

            ++current_;
            return true;
        }

        constexpr char peek() const {
            if (at_end()) return '\0';
            return source_[current_];
        }

        constexpr char peek_next() const {
            if (current_ + 1 >= source_.size()) return '\0';
            return source_[current_ + 1];
        }

        static constexpr bool is_alpha(char c) {
            return (c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z') ||
                c == '_';
        }

        static constexpr bool is_alphanumeric(char c) {
            return is_alpha(c) || is_digit(c);
        }

        static constexpr bool is_digit(char c) {
            return c >= '0' && c <= '9';
        }

        constexpr bool at_end() const {
            return current_ >= source_.size();
        }

        constexpr char advance() {
            return source_[current_++];
        }

        constexpr void add_token(token_type type, literal_t literal = {}) {
            auto text = source_.substr(start_, current_ - start_);
            tokens_.push_back({ .type_ = type, .lexeme_ = text, .literal_ = std::move(literal), .line_ = line_, });
        }

        std::string source_;
        std::vector<token> tokens_;
        std::vector<error> errors_;
        int start_ = 0;
        int current_ = 0;
        int line_ = 1;
    };
}