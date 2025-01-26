#pragma once

#include "token_type.h"
#include "string_ct.h"
#include "numbers.h"
#include "ct.h"

namespace ctlox {
    struct none {};
    struct nil {};

    template <int _location, string_ct _message>
    struct error_ct {};

    template <int _location, token_type _type, string_ct _lexeme, auto _literal = none{} >
    struct token_ct {
        static constexpr inline auto location = _location;
        static constexpr inline auto type = _type;
        static constexpr inline auto lexeme = _lexeme;
        static constexpr inline auto literal = _literal;
    };

    struct base_scanner_ct {
    protected:
        // Eveything that doesn't depend on scanner_ct's template argument goes here

        static constexpr inline bool is_alpha(char c) {
            return (c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z') ||
                c == '_';
        }

        static constexpr inline bool is_digit(char c) {
            return c >= '0' && c <= '9';
        }

        static constexpr inline bool is_alphanumeric(char c) {
            return is_alpha(c) || is_digit(c);
        }

        enum class char_class {
            single,
            single_maybe_equal,
            slash_or_comment,
            whitespace,
            string,
            number,
            identifier,
            eof,
            unknown,
        };

        static constexpr inline char_class classify_char(char c) {
            switch (c) {
            case '(':
            case ')':
            case '{':
            case '}':
            case ',':
            case '.':
            case '-':
            case '+':
            case ';':
            case '*':
                return char_class::single;
            case '/':
                return char_class::slash_or_comment;
            case '!':
            case '=':
            case '<':
            case '>':
                return char_class::single_maybe_equal;
            case ' ':
            case '\r':
            case '\n':
            case '\t':
                return char_class::whitespace;
            case '"':
                return char_class::string;
            case '\0':
                return char_class::eof;
            default:
                if (is_digit(c)) { return char_class::number; }
                if (is_alpha(c)) { return char_class::identifier; }
                return char_class::unknown;
            }
        }

        static constexpr inline token_type identify_single(char c) {
            switch (c) {
            case '(': return token_type::left_paren;
            case ')': return token_type::right_paren;
            case '{': return token_type::left_brace;
            case '}': return token_type::right_brace;
            case ',': return token_type::comma;
            case '.': return token_type::dot;
            case '-': return token_type::minus;
            case '+': return token_type::plus;
            case ';': return token_type::semicolon;
            case '*': return token_type::star;
            case '/': return token_type::slash;
            case '!': return token_type::bang;
            case '=': return token_type::equal;
            case '<': return token_type::less;
            case '>': return token_type::greater;
            default: return token_type::eof;
            }
        }

        static constexpr inline token_type identify_single_equal(char c) {
            switch (c) {
            case '!': return token_type::bang_equal;
            case '=': return token_type::equal_equal;
            case '<': return token_type::less_equal;
            case '>': return token_type::greater_equal;
            default: return token_type::eof;
            }
        }

        static constexpr inline token_type identify_keyword(std::string_view s) {
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

            const auto it = std::ranges::find(keywords, s, [](const auto& p) { return p.first; });
            if (it != keywords.end()) {
                return it->second;
            }
            return token_type::identifier;
        }

        template <token_type type>
        static constexpr inline auto keyword_literal = none{};
        template <>
        static constexpr inline auto keyword_literal<token_type::_false> = false;
        template <>
        static constexpr inline auto keyword_literal<token_type::_true> = true;
        template <>
        static constexpr inline auto keyword_literal<token_type::_nil> = nil{};
    };

    template <string_ct s>
    struct scan_ct : private base_scanner_ct {
    private:
        static constexpr inline bool at_end(int location) {
            return location >= s.size();
        }

        static constexpr inline char at(int location) {
            return at_end(location) ? '\0' : s[location];
        }

        static constexpr inline char_class classify_at(int location) {
            return classify_char(at(location));
        }

        static constexpr inline int find_end_of_number(int location) {
            while (is_digit(at(location))) ++location;
            if (at(location) == '.' && is_digit(at(location + 1))) {
                ++location;
                while (is_digit(at(location))) ++location;
            }

            return location;
        }

        static constexpr inline int find_end_of_identifier(int location) {
            while (is_alphanumeric(at(location))) ++location;
            return location;
        }

        template <int _location, char_class _class = classify_at(_location)>
        struct scan_token_at {
            // catch-all: unknown character
            template <typename C, typename... Tokens>
            using f = scan_token_at<_location + 1>::template f<
                C, Tokens...,
                error_ct<_location, "Unexpected character.">>;
        };

        template <int _location>
        struct scan_token_at<_location, char_class::eof> {
            template <typename C, typename... Tokens>
            using f = calln<
                C, Tokens...,
                token_ct<_location, token_type::eof, "">>;
        };

        template <int _location>
        struct scan_token_at<_location, char_class::whitespace> {
            template <typename C, typename... Tokens>
            using f = scan_token_at<_location + 1>::template f<C, Tokens...>;
        };

        template <int _location>
        struct scan_token_at<_location, char_class::single> {
            template <typename C, typename... Tokens>
            using f = scan_token_at<_location + 1>::template f<
                C, Tokens...,
                token_ct<_location, identify_single(_location), s.template substr<_location, _location + 1>()>
            >;
        };

        template <int _location>
        struct scan_token_at<_location, char_class::single_maybe_equal> {
            template <typename C, typename... Tokens>
            using f = scan_token_at<_location + 1>::template f<
                C, Tokens...,
                token_ct<_location, identify_single(_location), s.template substr<_location, _location + 1>()>
            >;
        };

        template <int _location>
            requires(at(_location + 1) == '=')
        struct scan_token_at<_location, char_class::single_maybe_equal> {
            template <typename C, typename... Tokens>
            using f = scan_token_at<_location + 2>::template f<
                C, Tokens...,
                token_ct<_location, identify_single_equal(_location), s.template substr<_location, _location + 2>()>
            >;
        };

        template <int _location>
        struct scan_token_at<_location, char_class::slash_or_comment> {
            template <typename C, typename... Tokens>
            using f = scan_token_at<_location + 1>::template f<
                C, Tokens...,
                token_ct<_location, identify_single(_location), s.template substr<_location, _location + 1>()>
            >;
        };

        template <int _location>
            requires(at(_location + 1) == '/')
        struct scan_token_at<_location, char_class::slash_or_comment> {
            template <typename C, typename... Tokens>
            using f = scan_token_at<s.find_next(_location, '\n')>::template f<C, Tokens...>;
        };

        template <int _location>
        struct scan_token_at<_location, char_class::string> {
            static constexpr inline auto end = s.find_next(_location + 1, '"') + 1;
            static constexpr inline auto lexeme = s.template substr<_location, end>();
            static constexpr inline auto str = s.template substr<_location + 1, end - 1>();

            template <typename C, typename... Tokens>
            using f = scan_token_at<s.find_next(_location + 1, '"') + 1>::template f<
                C, Tokens...,
                token_ct<_location, token_type::string, lexeme, str>
            >;
        };

        template <int _location>
            requires(at_end(s.find_next(_location + 1, '"')))
        struct scan_token_at<_location, char_class::string> {
            template <typename C, typename... Tokens>
            using f = scan_token_at<s.find_next(_location + 1, '"') + 1>::template f<
                C, Tokens...,
                error_ct<_location, "Unterminated string">
            >;
        };

        template <int _location>
        struct scan_token_at<_location, char_class::number> {
            static constexpr inline auto end = find_end_of_number(_location);
            static constexpr inline auto lexeme = s.template substr<_location, end>();

            template <typename C, typename... Tokens>
            using f = scan_token_at<end>::template f<
                C, Tokens...,
                token_ct<_location, token_type::number, lexeme, parse_double(lexeme)>
            >;
        };

        template <int _location>
        struct scan_token_at<_location, char_class::identifier> {
            static constexpr inline auto end = find_end_of_identifier(_location);
            static constexpr inline auto lexeme = s.template substr<_location, end>();
            static constexpr inline auto type = identify_keyword(lexeme);

            template <typename C, typename... Tokens>
            using f = scan_token_at<end>::template f<
                C, Tokens...,
                token_ct<_location, type, lexeme, keyword_literal<type>>
            >;
        };

    public:
        using has_f0 = void;

        template <typename C>
        using f0 = scan_token_at<0>::template f<C>;
    };

    namespace tests {

        using output_1 = call<compose<
            scan_ct< R"("sup" >= (3 / 2) > "bye" // signing off)">,
            to_list,
            returned>>;

        static_assert(output_1::size == 10);

        using scanned = call<scan_ct<"var x = false nil true">>;

        template <std::size_t I>
        using scanned_at = call<compose<scanned, at<I>, returned>>;

        static_assert(scanned_at<0>::type == token_type::_var);
        static_assert(scanned_at<0>::lexeme == "var");
        static_assert(std::convertible_to<decltype(scanned_at<0>::literal), none>);
        //call<compose<given_pack<decltype(scanned_at<0>::literal), char>, to_error>>;

        static_assert(scanned_at<1>::type == token_type::identifier);
        static_assert(scanned_at<1>::lexeme == "x");

        static_assert(scanned_at<3>::type == token_type::_false);
        static_assert(scanned_at<3>::lexeme == "false");
        static_assert(scanned_at<3>::literal == false);

        static_assert(scanned_at<4>::type == token_type::_nil);
        static_assert(scanned_at<4>::lexeme == "nil");
        static_assert(std::convertible_to<decltype(scanned_at<4>::literal), nil>);

        static_assert(scanned_at<5>::type == token_type::_true);
        static_assert(scanned_at<5>::lexeme == "true");
        static_assert(scanned_at<5>::literal == true);
    }


    //using x = printout<output>;
};
