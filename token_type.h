#pragma once

namespace ctlox {
    enum class token_type {
        left_paren, right_paren, left_brace, right_brace,
        comma, dot, minus, plus, semicolon, slash, star,

        bang, bang_equal,
        equal, equal_equal,
        greater, greater_equal,
        less, less_equal,

        identifier, string, number,

        _and, _class, _else, _false, _fun, _for, _if, _nil, _or,
        _print, _return, _super, _this, _true, _var, _while,

        eof,
    };
}