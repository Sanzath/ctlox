#include <ctlox/v2/flattener.hpp>

namespace test_v2::test_flattener {

constexpr bool scratch_tests() {
    using namespace ctlox::v2;

    std::string s = "foo";
    static_assert(std::convertible_to<flat_expression_stmt, flat_stmt_t>);
    static_assert(std::convertible_to<flat_expression_stmt&&, flat_stmt_t>);
    static_assert(std::constructible_from<flat_stmt_t, flat_expression_stmt>);
    flat_stmt_t stmt = flat_expression_stmt {};
    flat_expr_t expr = flat_variable_expr {};

    return true;
}

}  // namespace test_v2::test_flattener