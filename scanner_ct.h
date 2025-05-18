#pragma once

#include "ct.h"
#include "numbers.h"
#include "string_ct.h"
#include "types.h"

namespace ctlox {

struct base_scanner_ct {
protected:
    // Eveything that doesn't depend on scanner_ct's template argument goes here

    static constexpr inline bool is_alpha(char c)
    {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
    }

    static constexpr inline bool is_digit(char c)
    {
        return c >= '0' && c <= '9';
    }

    static constexpr inline bool is_alphanumeric(char c)
    {
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

    static constexpr inline char_class classify_char(char c)
    {
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

    static constexpr inline token_type identify_single(char c)
    {
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

    static constexpr inline token_type identify_single_equal(char c)
    {
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

    static constexpr inline token_type identify_keyword(std::string_view s)
    {
        constexpr auto keywords = std::to_array<std::pair<std::string_view, token_type>>({
            { "and", token_type::_and },
            { "class", token_type::_class },
            { "else", token_type::_else },
            { "false", token_type::_false },
            { "for", token_type::_for },
            { "fun", token_type::_fun },
            { "if", token_type::_if },
            { "nil", token_type::_nil },
            { "or", token_type::_or },
            { "print", token_type::_print },
            { "return", token_type::_return },
            { "super", token_type::_super },
            { "this", token_type::_this },
            { "true", token_type::_true },
            { "var", token_type::_var },
            { "while", token_type::_while },
        });

        const auto it = std::ranges::find(keywords, s, [](const auto& p) { return p.first; });
        if (it != keywords.end()) {
            return it->second;
        }
        return token_type::identifier;
    }

    // error "strings" (will be displayed more or less properly with errored
    template <std::size_t _location>
    struct _at_location_ {
        template <char c>
        struct _unexpected_character_ { };

        struct _unterminated_string_ { };
    };

    template <token_type type>
    static constexpr inline auto keyword_literal = none;
    template <>
    constexpr auto keyword_literal<token_type::_false> = false;
    template <>
    constexpr auto keyword_literal<token_type::_true> = true;
    template <>
    constexpr auto keyword_literal<token_type::_nil> = nil;
};

template <string_ct s>
struct scan_ct : private base_scanner_ct {
private:
    static constexpr inline bool at_end(std::size_t location)
    {
        return location >= s.size();
    }

    static constexpr inline char at(std::size_t location)
    {
        return at_end(location) ? '\0' : s[location];
    }

    static constexpr inline char_class classify_at(std::size_t location)
    {
        return classify_char(at(location));
    }

    static constexpr inline std::size_t find_end_of_number(std::size_t location)
    {
        while (is_digit(at(location)))
            ++location;
        if (at(location) == '.' && is_digit(at(location + 1))) {
            ++location;
            while (is_digit(at(location)))
                ++location;
        }

        return location;
    }

    static constexpr inline std::size_t find_end_of_identifier(std::size_t location)
    {
        while (is_alphanumeric(at(location)))
            ++location;
        return location;
    }

    template <std::size_t _location, char_class _class = classify_at(_location)>
    struct scan_token_at {
        // catch-all: unknown character
        template <typename C, typename... Tokens>
        using f = scan_token_at<_location + 1>::template f<
            C, Tokens...,
            error_t<typename _at_location_<_location>::template _unexpected_character_<at(_location)>>>;
    };

    template <std::size_t _location>
    struct scan_token_at<_location, char_class::eof> {
        template <typename C, typename... Tokens>
        using f = calln<
            C, Tokens...,
            token_ct<_location, token_type::eof, "">>;
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
            token_ct<_location, identify_single(at(_location)), s.template substr<_location, _location + 1>()>>;
    };

    template <std::size_t _location>
    struct scan_token_at<_location, char_class::single_maybe_equal> {
        template <typename C, typename... Tokens>
        using f = scan_token_at<_location + 1>::template f<
            C, Tokens...,
            token_ct<_location, identify_single(at(_location)), s.template substr<_location, _location + 1>()>>;
    };

    template <std::size_t _location>
        requires(at(_location + 1) == '=')
    struct scan_token_at<_location, char_class::single_maybe_equal> {
        template <typename C, typename... Tokens>
        using f = scan_token_at<_location + 2>::template f<
            C, Tokens...,
            token_ct<_location, identify_single_equal(at(_location)), s.template substr<_location, _location + 2>()>>;
    };

    template <std::size_t _location>
    struct scan_token_at<_location, char_class::slash_or_comment> {
        template <typename C, typename... Tokens>
        using f = scan_token_at<_location + 1>::template f<
            C, Tokens...,
            token_ct<_location, identify_single(at(_location)), s.template substr<_location, _location + 1>()>>;
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
            token_ct<_location, token_type::string, lexeme, str>>;
    };

    template <std::size_t _location>
        requires(at_end(s.find_next(_location + 1, '"')))
    struct scan_token_at<_location, char_class::string> {
        template <typename C, typename... Tokens>
        using f = scan_token_at<s.find_next(_location + 1, '"') + 1>::template f<
            C, Tokens...,
            error_t<typename _at_location_<_location>::_unterminated_string_>>;
    };

    template <std::size_t _location>
    struct scan_token_at<_location, char_class::number> {
        static constexpr inline auto end = find_end_of_number(_location);
        static constexpr inline auto lexeme = s.template substr<_location, end>();

        template <typename C, typename... Tokens>
        using f = scan_token_at<end>::template f<
            C, Tokens...,
            token_ct<_location, token_type::number, lexeme, parse_double(lexeme)>>;
    };

    template <std::size_t _location>
    struct scan_token_at<_location, char_class::identifier> {
        static constexpr inline auto end = find_end_of_identifier(_location);
        static constexpr inline auto lexeme = s.template substr<_location, end>();
        static constexpr inline auto type = identify_keyword(lexeme);

        template <typename C, typename... Tokens>
        using f = scan_token_at<end>::template f<
            C, Tokens...,
            token_ct<_location, type, lexeme, keyword_literal<type>>>;
    };

public:
    using has_f0 = void;

    template <typename C>
    using f0 = scan_token_at<0>::template f<C>;
};

namespace tests {
    using tokens_1 = run<
        scan_ct<R"("sup" >= (3 / 2) > "bye" // signing off)">,
        listed>;

    static_assert(tokens_1::size == 10);

    using tokens_2 = run<
        scan_ct<"var x = false nil true">,
        listed>;

    using expected_tokens_2 = list<
        token_ct<0, token_type::_var, "var">,
        token_ct<4, token_type::identifier, "x">,
        token_ct<6, token_type::equal, "=">,
        token_ct<8, token_type::_false, "false", false>,
        token_ct<14, token_type::_nil, "nil", nil>,
        token_ct<18, token_type::_true, "true", true>,
        token_ct<22, token_type::eof, "">>;

    static_assert(std::same_as<tokens_2, expected_tokens_2>);
}
};
