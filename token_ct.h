#pragma once

#include "token_type.h"
#include "string_ct.h"

namespace ctlox {
    struct none_t {
        constexpr bool operator==(const none_t&) const noexcept { return true; }

        template <typename T>
        constexpr bool operator==(const T&) const noexcept { return false; }
    };

    static constexpr inline none_t none;

    struct nil_t {};
    static constexpr inline nil_t nil;

    template <std::size_t _location, token_type _type, string_ct _lexeme, auto _literal = none>
    struct token_ct {
        static constexpr inline auto location = _location;
        static constexpr inline auto type = _type;
        static constexpr inline auto lexeme = _lexeme;
        static constexpr inline auto literal = _literal;
    };

}