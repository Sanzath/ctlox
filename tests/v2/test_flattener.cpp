#include "framework.hpp"

#include <ctlox/v2/flattener.hpp>
#include <ctlox/v2/parser.hpp>
#include <ctlox/v2/scanner.hpp>

namespace test_v2::test_flattener {



constexpr bool scratch_tests() {
    using namespace ctlox::v2;

    constexpr auto source = R"(
var foo = (12 + 13) / 2;
{
    print foo;
    var space = " ";
    foo = "Hello," + space + "world!";
}
print foo;
)";

    const auto flat_tree = flatten(parse(scan(source)));

    for (const auto& statement : flat_tree.root_range()) {
        if (auto* var_stmt = statement.get_if<flat_var_stmt>()) {
            flat_tree[var_stmt->initializer_].visit([](const flat_expr_t&) { });
        }
        else if (auto* block_stmt = statement.get_if<flat_block_stmt>()) {
            flat_tree.range(block_stmt->statements_);
        }
    }

    expect(flat_tree.root_block_.first_.i == 0);
    expect(flat_tree.root_block_.last_.i == 3);

    expect(flat_tree.statements_.size() == 6);
    expect(flat_tree.expressions_.size() == 15);

    return true;
}

static_assert(scratch_tests());

}  // namespace test_v2::test_flattener