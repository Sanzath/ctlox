#include <ranges>
#include <algorithm>
#include <array>
#include <iostream>
#include <variant>
#include <format>
#include <vector>
#include <charconv>

#include "ct.h"
#include "scanner_ct.h"
#include "parser_ct.h"

namespace ctlox {
    struct detect_errors {

        template <typename C, typename... Ts>
        using fn = void;
    };

    template <string_ct s>
    using lox = compose<
        scan_ct<s>,
        detect_errors
    >;
}

int main()
{
}
