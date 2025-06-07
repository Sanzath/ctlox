#pragma once

#include <algorithm>
#include <array>
#include <string_view>
#include <utility>

namespace ctlox::inline common {
enum class token_type {
    // Single-character tokens
    left_paren,
    right_paren,
    left_brace,
    right_brace,
    comma,
    dot,
    minus,
    plus,
    semicolon,
    slash,
    star,

    // One or two character tokens
    bang,
    bang_equal,
    equal,
    equal_equal,
    greater,
    greater_equal,
    less,
    less_equal,

    // Literals
    identifier,
    string,
    number,

    // Keywords
    _and,
    _break,
    _class,
    _else,
    _false,
    _fun,
    _for,
    _if,
    _nil,
    _or,
    _print,
    _return,
    _super,
    _this,
    _true,
    _var,
    _while,

    eof,
};

constexpr token_type identify_keyword(std::string_view s) {
    constexpr auto keywords = std::to_array<std::pair<std::string_view, token_type>>({
        { "and", token_type::_and },
        { "break", token_type::_break },
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

}  // namespace ctlox::inline common