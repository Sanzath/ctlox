#pragma once

// clang-format off
#include <ctlox/common/string.hpp>
#include <ctlox/v2/scanner.hpp>
#include <ctlox/v2/parser.hpp>
#include <ctlox/v2/serializer.hpp>
#include <ctlox/v2/code_generator.hpp>
// clang-format on

namespace ctlox::v2 {

template <string source>
constexpr auto compile() {
    constexpr auto generate_ast = [] { return parse(scan(source)); };
    static constexpr auto ast = static_serialize<generate_ast>();
    return generate_code<ast>();
}

template <string source>
constexpr auto compile_v = compile<source>();

inline namespace literals {
    template <string source>
    constexpr auto operator ""_lox() {
        return compile<source>();
    }
}

}  // namespace ctlox::v2

#define CTLOX_DEFAULT_V2
#ifdef CTLOX_DEFAULT_V2
namespace ctlox {
using namespace v2;
}
#endif