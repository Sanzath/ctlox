#pragma once

#include <ctlox/common/characters.hpp>
#include <ctlox/common/numbers.hpp>
#include <ctlox/common/string.hpp>
#include <ctlox/v1/ct.hpp>
#include <ctlox/v1/types.hpp>

namespace ctlox::v1 {

struct base_scanner_ct {
protected:
    // Eveything that doesn't depend on scanner_ct's template argument goes here
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
            if (is_digit(c)) {
                return char_class::number;
            }
            if (is_alpha(c)) {
                return char_class::identifier;
            }
            return char_class::unknown;
        }
    }

    static constexpr inline token_type identify_single(char c) {
        switch (c) {
        case '(':
            return token_type::left_paren;
        case ')':
            return token_type::right_paren;
        case '{':
            return token_type::left_brace;
        case '}':
            return token_type::right_brace;
        case ',':
            return token_type::comma;
        case '.':
            return token_type::dot;
        case '-':
            return token_type::minus;
        case '+':
            return token_type::plus;
        case ';':
            return token_type::semicolon;
        case '*':
            return token_type::star;
        case '/':
            return token_type::slash;
        case '!':
            return token_type::bang;
        case '=':
            return token_type::equal;
        case '<':
            return token_type::less;
        case '>':
            return token_type::greater;
        default:
            return token_type::eof;
        }
    }

    static constexpr inline token_type identify_single_equal(char c) {
        switch (c) {
        case '!':
            return token_type::bang_equal;
        case '=':
            return token_type::equal_equal;
        case '<':
            return token_type::less_equal;
        case '>':
            return token_type::greater_equal;
        default:
            return token_type::eof;
        }
    }

    template <token_type type>
    static constexpr inline auto keyword_literal = none;
    template <>
    constexpr auto keyword_literal<token_type::_false> = false;
    template <>
    constexpr auto keyword_literal<token_type::_true> = true;
    template <>
    constexpr auto keyword_literal<token_type::_nil> = nil;
};

template <string s>
struct scan_ct : private base_scanner_ct {
private:
    static constexpr inline bool at_end(std::size_t location) {
        return location >= s.size();
    }

    static constexpr inline char at(std::size_t location) {
        return at_end(location) ? '\0' : s[location];
    }

    static constexpr inline char_class classify_at(std::size_t location) {
        return classify_char(at(location));
    }

    static constexpr inline std::size_t find_end_of_number(std::size_t location) {
        while (is_digit(at(location)))
            ++location;
        if (at(location) == '.' && is_digit(at(location + 1))) {
            ++location;
            while (is_digit(at(location)))
                ++location;
        }

        return location;
    }

    static constexpr inline std::size_t find_end_of_identifier(std::size_t location) {
        while (is_alphanumeric(at(location)))
            ++location;
        return location;
    }

    template <std::size_t _location, char_class _class = classify_at(_location)>
    struct scan_token_at {
        static_assert(false, "Unknown character");
    };

    template <std::size_t _location>
    struct scan_token_at<_location, char_class::eof> {
        template <typename C, typename... Tokens>
        using f = calln<
            C, Tokens...,
            token_t<_location, token_type::eof, "">>;
    };

    template <std::size_t _location>
    struct scan_token_at<_location, char_class::whitespace> {
        template <typename C, typename... Tokens>
        using f = scan_token_at<_location + 1>::template f<C, Tokens...>;
    };

    template <std::size_t _location>
    struct scan_token_at<_location, char_class::single> {
        template <typename C, typename... Tokens>
        using f = scan_token_at<_location + 1>::template f<
            C, Tokens...,
            token_t<_location, identify_single(at(_location)), s.template substr<_location, _location + 1>()>>;
    };

    template <std::size_t _location>
    struct scan_token_at<_location, char_class::single_maybe_equal> {
        template <typename C, typename... Tokens>
        using f = scan_token_at<_location + 1>::template f<
            C, Tokens...,
            token_t<_location, identify_single(at(_location)), s.template substr<_location, _location + 1>()>>;
    };

    template <std::size_t _location>
        requires(at(_location + 1) == '=')
    struct scan_token_at<_location, char_class::single_maybe_equal> {
        template <typename C, typename... Tokens>
        using f = scan_token_at<_location + 2>::template f<
            C, Tokens...,
            token_t<_location, identify_single_equal(at(_location)), s.template substr<_location, _location + 2>()>>;
    };

    template <std::size_t _location>
    struct scan_token_at<_location, char_class::slash_or_comment> {
        template <typename C, typename... Tokens>
        using f = scan_token_at<_location + 1>::template f<
            C, Tokens...,
            token_t<_location, identify_single(at(_location)), s.template substr<_location, _location + 1>()>>;
    };

    template <std::size_t _location>
        requires(at(_location + 1) == '/')
    struct scan_token_at<_location, char_class::slash_or_comment> {
        template <typename C, typename... Tokens>
        using f = scan_token_at<s.find_next(_location, '\n')>::template f<C, Tokens...>;
    };

    template <std::size_t _location>
    struct scan_token_at<_location, char_class::string> {
        static constexpr inline auto end = s.find_next(_location + 1, '"') + 1;
        static constexpr inline auto lexeme = s.template substr<_location, end>();
        static constexpr inline auto str = s.template substr<_location + 1, end - 1>();

        template <typename C, typename... Tokens>
        using f = scan_token_at<s.find_next(_location + 1, '"') + 1>::template f<
            C, Tokens...,
            token_t<_location, token_type::string, lexeme, str>>;
    };

    template <std::size_t _location>
        requires(at_end(s.find_next(_location + 1, '"')))
    struct scan_token_at<_location, char_class::string> {
        static_assert(false, "Unterminated string");
    };

    template <std::size_t _location>
    struct scan_token_at<_location, char_class::number> {
        static constexpr inline auto end = find_end_of_number(_location);
        static constexpr inline auto lexeme = s.template substr<_location, end>();

        template <typename C, typename... Tokens>
        using f = scan_token_at<end>::template f<
            C, Tokens...,
            token_t<_location, token_type::number, lexeme, parse_double(lexeme)>>;
    };

    template <std::size_t _location>
    struct scan_token_at<_location, char_class::identifier> {
        static constexpr inline auto end = find_end_of_identifier(_location);
        static constexpr inline auto lexeme = s.template substr<_location, end>();
        static constexpr inline auto type = identify_keyword(lexeme);

        template <typename C, typename... Tokens>
        using f = scan_token_at<end>::template f<
            C, Tokens...,
            token_t<_location, type, lexeme, keyword_literal<type>>>;
    };

public:
    using has_f0 = void;

    template <typename C>
    using f0 = scan_token_at<0>::template f<C>;
};
};  // namespace ctlox::v1
