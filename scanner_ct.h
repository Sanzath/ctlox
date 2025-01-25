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
    };

    template <string_ct s>
    struct scanner_ct : private base_scanner_ct {
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
            using f = C::template f<
                // C argument is dropped... oh.....
                Tokens...,
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
            template <typename C, typename... Tokens>
            using f = scan_token_at<s.find_next(_location + 1, '"') + 1>::template f<
                C, Tokens...,
                token_ct<
                _location,
                token_type::string,
                s.template substr<_location, s.find_next(_location + 1, '"') + 1>(),
                s.template substr<_location + 1, s.find_next(_location + 1, '"')>()>
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
            template <typename C, typename... Tokens>
            using f = scan_token_at<find_end_of_number(_location)>::template f<
                C, Tokens...,
                token_ct<
                _location,
                token_type::number,
                s.template substr<_location, find_end_of_number(_location)>(),
                parse_double(s.template substr<_location, find_end_of_number(_location)>())>
            >;
        };

        // TODO: identifier (& keywords)

    public:
        template <typename C>
        using f = scan_token_at<0>::template f<C>;
    };

    template <string_ct s, typename C>
    using scan_ct = scanner_ct<s>::template f<C>;

    namespace tests {

        //using output_1 = scan_ct<R"("sup" >= (3 / 2) > "bye" // signing off)", to_list>;

        //static_assert(output_1::size == 10);

        //using output_2 =
        //    identity::template f<
        //    //passthrough,
        //    at<0>::template f<
        //    passthrough,
        //    scanner_ct<R"()">::template f<passthrough>
        //    >
        //    >;

        //using output_2 = scanner_ct<"">
        //    ::template f<chained<at<0>, identity>>;
        //::template f<

        //static_assert(output_2::type == token_type::eof);

        //using x = printout<output>;
    }


    //using x = printout<output>;
};
