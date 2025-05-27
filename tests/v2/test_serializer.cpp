#include "framework.hpp"

#include <ctlox/v2/parser.hpp>
#include <ctlox/v2/scanner.hpp>
#include <ctlox/v2/serializer.hpp>

#include <ranges>
#include <span>
#include <string_view>

namespace test_v2::test_serializer {

using namespace std::string_view_literals;

template <typename T, typename NodeType>
constexpr const T& expect_holds(const ctlox::v2::flat_program& program, ctlox::v2::flat_ptr<NodeType> node_ptr) {
    expect(node_ptr != ctlox::v2::flat_nullptr);

    if (const T* node = program[node_ptr].template get_if<T>()) {
        return *node;
    }

    throw std::logic_error("Node holds incorrect type");
}

constexpr auto null_expr() {
    return [](const ctlox::v2::flat_program& program, ctlox::v2::flat_expr_ptr node_ptr) {
        expect(node_ptr == ctlox::v2::flat_nullptr);
    };
}

constexpr auto assign_expr(std::string_view name, auto check_value) {
    return [=](const ctlox::v2::flat_program& program, ctlox::v2::flat_expr_ptr node_ptr) {
        const auto& assign_expr = expect_holds<ctlox::v2::flat_assign_expr>(program, node_ptr);
        expect(assign_expr.name_.lexeme_ == name);
        check_value(program, assign_expr.value_);
    };
}

constexpr auto binary_expr(ctlox::token_type oper, auto check_left, auto check_right) {
    return [=](const ctlox::v2::flat_program& program, ctlox::v2::flat_expr_ptr node_ptr) {
        const auto& binary_expr = expect_holds<ctlox::v2::flat_binary_expr>(program, node_ptr);
        expect(binary_expr.operator_.type_ == oper);
        check_left(program, binary_expr.left_);
        check_right(program, binary_expr.right_);
    };
}

constexpr auto grouping_expr(auto check_expr) {
    return [=](const ctlox::v2::flat_program& program, ctlox::v2::flat_expr_ptr node_ptr) {
        const auto& grouping_expr = expect_holds<ctlox::v2::flat_grouping_expr>(program, node_ptr);
        check_expr(program, grouping_expr.expr_);
    };
}

constexpr auto literal_expr(const ctlox::v2::literal_t& literal) {
    return [=](const ctlox::v2::flat_program& program, ctlox::v2::flat_expr_ptr node_ptr) {
        const auto& literal_expr = expect_holds<ctlox::v2::flat_literal_expr>(program, node_ptr);
        expect(literal_expr.value_ == literal);
    };
}

constexpr auto unary_expr(ctlox::token_type oper, auto check_right) {
    return [=](const ctlox::v2::flat_program& program, ctlox::v2::flat_expr_ptr node_ptr) {
        const auto& unary_expr = expect_holds<ctlox::v2::flat_unary_expr>(program, node_ptr);
        expect(unary_expr.operator_.type_ == oper);
        check_right(program, unary_expr.right_);
    };
}

constexpr auto variable_expr(std::string_view name) {
    return [=](const ctlox::v2::flat_program& program, ctlox::v2::flat_expr_ptr node_ptr) {
        const auto& variable_expr = expect_holds<ctlox::v2::flat_variable_expr>(program, node_ptr);
        expect(variable_expr.name_.lexeme_ == name);
    };
}

constexpr auto block_stmt(auto... check_statements) {
    return [=](const ctlox::v2::flat_program& program, ctlox::v2::flat_stmt_ptr node_ptr) {
        const auto& block_stmt = expect_holds<ctlox::v2::flat_block_stmt>(program, node_ptr);
        const auto& statements = block_stmt.statements_;

        expect(statements.size() == sizeof...(check_statements));

        auto it = statements.begin();
        (check_statements(program, *it++), ...);

        expect(it == statements.end());
    };
}

constexpr auto expression_stmt(auto check_expression) {
    return [=](const ctlox::v2::flat_program& program, ctlox::v2::flat_stmt_ptr node_ptr) {
        const auto& expression_stmt = expect_holds<ctlox::v2::flat_expression_stmt>(program, node_ptr);
        check_expression(program, expression_stmt.expression_);
    };
}

constexpr auto print_stmt(auto check_expression) {
    return [=](const ctlox::v2::flat_program& program, ctlox::v2::flat_stmt_ptr node_ptr) {
        const auto& print_stmt = expect_holds<ctlox::v2::flat_print_stmt>(program, node_ptr);
        check_expression(program, print_stmt.expression_);
    };
}

constexpr auto var_stmt(std::string_view name, auto check_initializer) {
    return [=](const ctlox::v2::flat_program& program, ctlox::v2::flat_stmt_ptr node_ptr) {
        const auto& var_stmt = expect_holds<ctlox::v2::flat_var_stmt>(program, node_ptr);
        expect(var_stmt.name_.lexeme_ == name);
        check_initializer(program, var_stmt.initializer_);
    };
}

constexpr bool test_program(std::string_view source, auto... check_statements) {
    const ctlox::v2::flat_program program = ctlox::v2::serialize(ctlox::v2::parse(ctlox::v2::scan(source)));
    const auto& statements = program.root_block_;

    expect(statements.size() == sizeof...(check_statements));

    auto it = statements.begin();
    (check_statements(program, *it++), ...);

    expect(it == statements.end());

    return true;
}

// clang-format off
static_assert(test_program("var foo;",
    var_stmt("foo", null_expr())
));

static_assert(test_program("var foo = 15;",
    var_stmt("foo", literal_expr(15.0))
));

static_assert(test_program(
    "var i; var j; { i = 2; j = i; } print i * j;",
    var_stmt("i", null_expr()),
    var_stmt("j", null_expr()),
    block_stmt(
        expression_stmt(assign_expr("i", literal_expr(2.0))),
        expression_stmt(assign_expr("j", variable_expr("i")))),
    print_stmt(binary_expr(ctlox::token_type::star,
        variable_expr("i"),
        variable_expr("j")))
));


static_assert(test_program("!((((a)) * (b + c)) == 17);",
    expression_stmt(unary_expr(ctlox::token_type::bang,
        grouping_expr(
            binary_expr(ctlox::token_type::equal_equal,
                grouping_expr(
                    binary_expr(ctlox::token_type::star,
                        grouping_expr(
                            grouping_expr(
                                variable_expr("a"sv))),
                        grouping_expr(
                            binary_expr(ctlox::token_type::plus,
                                variable_expr("b"sv),
                                variable_expr("c"sv))))),
                literal_expr(17.0)))))
));
// clang-format on

constexpr bool test_sizes() {
    constexpr auto source = R"(
var foo = (12 + 13) / 2;
{
    print foo;
    var space = " ";
    foo = "Hello," + space + "world!";
}
print !foo;
)";

    const auto program = ctlox::v2::serialize(ctlox::v2::parse(ctlox::v2::scan(source)));

    expect(program.root_block_.first_.i == 0);
    expect(program.root_block_.last_.i == 3);

    expect(program.statements_.size() == 6);
    expect(program.expressions_.size() == 16);

    return true;
}

static_assert(test_sizes());

}  // namespace test_v2::test_flattener