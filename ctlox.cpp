#include <algorithm>

#include "ct.h"
#include "interpreter_ct.h"
#include "parser_ct.h"
#include "scanner_ct.h"

namespace ctlox {
struct detect_errors {
    template <typename T>
    static constexpr inline bool is_token = false;
    template <std::size_t _location, token_type _type, string_ct _lexeme, auto _literal>
    static constexpr inline bool is_token<token_ct<_location, _type, _lexeme, _literal>> = true;

    template <typename... Ts>
    struct collect_tokens {
        static_assert(false, "???");
    };

    template <typename... Ts>
    struct collect_nontokens {
        static_assert(false, "???");
    };

    template <typename T, typename... Ts>
        requires(is_token<T>)
    struct collect_tokens<T, Ts...> {
        template <typename C, typename... Tokens>
        using f = collect_tokens<Ts...>::template f<
            C, Tokens..., T>;
    };

    template <typename T, typename... Ts>
        requires(!is_token<T>)
    struct collect_tokens<T, Ts...> {
        // non-token found: switch to error path, collecting only non-tokens
        template <typename, typename...>
        using f = collect_nontokens<Ts...>::template f<T>;
    };

    template <>
    struct collect_tokens<> {
        template <typename C, typename... Tokens>
        using f = calln<C, Tokens...>;
    };

    template <typename T, typename... Ts>
        requires(is_token<T>)
    struct collect_nontokens<T, Ts...> {
        template <typename... NonTokens>
        using f = collect_nontokens<Ts...>::template f<NonTokens...>;
    };

    template <typename T, typename... Ts>
        requires(!is_token<T>)
    struct collect_nontokens<T, Ts...> {
        template <typename... NonTokens>
        using f = collect_nontokens<Ts...>::template f<NonTokens..., T>;
    };

    template <>
    struct collect_nontokens<> {
        template <typename... NonTokens>
        using f = calln<errored, NonTokens...>;
    };

    using has_fn = void;
    template <accepts_pack C, typename... Ts>
    using fn = collect_tokens<Ts...>::template f<C>;
};

template <string_ct s>
using lox = compose<
    scan_ct<s>,
    detect_errors,
    parse_ct,
    interpret_ct>;
}

int main()
{
    using output = ctlox::run<
        ctlox::lox<R"(
            var foo = 10;
            print foo + 15.5;
            var foo;
            print foo;)">,
        ctlox::listed>;

    using namespace ctlox::literals;
    static_assert(std::is_same_v<
        output,
        ctlox::list<
            ctlox::value_t<25.5>,
            ctlox::value_t<ctlox::nil>>>);
}
