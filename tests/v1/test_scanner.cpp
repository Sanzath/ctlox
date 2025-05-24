#include <ctlox/v1/scanner.hpp>

namespace test_v1::test_scanner {
using namespace ctlox::common;
using namespace ctlox::v1;

using tokens_1 = run<
    scan_ct<R"("sup" >= (3 / 2) > "bye" // signing off)">,
    listed>;

static_assert(tokens_1::size == 10);

using tokens_2 = run<
    scan_ct<"var x = false nil true">,
    listed>;

using expected_tokens_2 = list<
    token_t<0, token_type::_var, "var">,
    token_t<4, token_type::identifier, "x">,
    token_t<6, token_type::equal, "=">,
    token_t<8, token_type::_false, "false", false>,
    token_t<14, token_type::_nil, "nil", nil>,
    token_t<18, token_type::_true, "true", true>,
    token_t<22, token_type::eof, "">>;

static_assert(std::same_as<tokens_2, expected_tokens_2>);
}