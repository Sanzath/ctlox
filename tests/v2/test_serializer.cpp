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
constexpr const T& expect_holds(const auto& ast, ctlox::v2::flat_ptr<NodeType> node_ptr) {
    expect(node_ptr != ctlox::v2::flat_nullptr);

    if (const T* node = ast[node_ptr].template get_if<T>()) {
        return *node;
    }

    throw std::logic_error("Node holds incorrect type");
}

constexpr auto null_stmt() {
    return [](const auto& ast, ctlox::v2::flat_stmt_ptr node_ptr) { expect(node_ptr == ctlox::v2::flat_nullptr); };
}

constexpr auto block_stmt(auto... check_statements) {
    return [=](const auto& ast, ctlox::v2::flat_stmt_ptr node_ptr) {
        const auto& block_stmt = expect_holds<ctlox::v2::flat_block_stmt>(ast, node_ptr);
        const auto& statements = block_stmt.statements_;

        expect(statements.size() == sizeof...(check_statements));

        auto it = statements.begin();
        (check_statements(ast, *it++), ...);

        expect(it == statements.end());
    };
}

constexpr auto expression_stmt(auto check_expression) {
    return [=](const auto& ast, ctlox::v2::flat_stmt_ptr node_ptr) {
        const auto& expression_stmt = expect_holds<ctlox::v2::flat_expression_stmt>(ast, node_ptr);
        check_expression(ast, expression_stmt.expression_);
    };
}

constexpr auto if_stmt(auto check_condition, auto check_then_branch, auto check_else_branch) {
    return [=](const auto& ast, ctlox::v2::flat_stmt_ptr node_ptr) {
        const auto& if_stmt = expect_holds<ctlox::v2::flat_if_stmt>(ast, node_ptr);
        check_condition(ast, if_stmt.condition_);
        check_then_branch(ast, if_stmt.then_branch_);
        check_else_branch(ast, if_stmt.else_branch_);
    };
}

constexpr auto var_stmt(std::string_view name, auto check_initializer) {
    return [=](const auto& ast, ctlox::v2::flat_stmt_ptr node_ptr) {
        const auto& var_stmt = expect_holds<ctlox::v2::flat_var_stmt>(ast, node_ptr);
        expect(var_stmt.name_.lexeme_ == name);
        check_initializer(ast, var_stmt.initializer_);
    };
}

constexpr auto while_stmt(auto check_condition, auto check_body) {
    return [=](const auto& ast, ctlox::v2::flat_stmt_ptr node_ptr) {
        const auto& while_stmt = expect_holds<ctlox::v2::flat_while_stmt>(ast, node_ptr);
        check_condition(ast, while_stmt.condition_);
        check_body(ast, while_stmt.body_);
    };
}

constexpr auto null_expr() {
    return [](const auto& ast, ctlox::v2::flat_expr_ptr node_ptr) { expect(node_ptr == ctlox::v2::flat_nullptr); };
}

constexpr auto assign_expr(std::string_view name, auto check_value) {
    return [=](const auto& ast, ctlox::v2::flat_expr_ptr node_ptr) {
        const auto& assign_expr = expect_holds<ctlox::v2::flat_assign_expr>(ast, node_ptr);
        expect(assign_expr.name_.lexeme_ == name);
        check_value(ast, assign_expr.value_);
    };
}

constexpr auto binary_expr(ctlox::token_type oper, auto check_left, auto check_right) {
    return [=](const auto& ast, ctlox::v2::flat_expr_ptr node_ptr) {
        const auto& binary_expr = expect_holds<ctlox::v2::flat_binary_expr>(ast, node_ptr);
        expect(binary_expr.operator_.type_ == oper);
        check_left(ast, binary_expr.left_);
        check_right(ast, binary_expr.right_);
    };
}

constexpr auto call_expr(auto check_callee, auto... check_args) {
    return [=](const auto& ast, ctlox::v2::flat_expr_ptr node_ptr) {
        const auto call_expr = expect_holds<ctlox::v2::flat_call_expr>(ast, node_ptr);
        check_callee(ast, call_expr.callee_);

        auto it = call_expr.arguments_.begin();
        (check_args(ast, *it++), ...);
    };
}

constexpr auto grouping_expr(auto check_expr) {
    return [=](const auto& ast, ctlox::v2::flat_expr_ptr node_ptr) {
        const auto& grouping_expr = expect_holds<ctlox::v2::flat_grouping_expr>(ast, node_ptr);
        check_expr(ast, grouping_expr.expr_);
    };
}

constexpr auto literal_expr(const ctlox::v2::literal_t& literal) {
    return [=](const auto& ast, ctlox::v2::flat_expr_ptr node_ptr) {
        const auto& literal_expr = expect_holds<ctlox::v2::flat_literal_expr>(ast, node_ptr);
        expect(literal_expr.value_ == literal);
    };
}

constexpr auto logical_expr(ctlox::token_type oper, auto check_left, auto check_right) {
    return [=](const auto& ast, ctlox::v2::flat_expr_ptr node_ptr) {
        const auto& logical_expr = expect_holds<ctlox::v2::flat_logical_expr>(ast, node_ptr);
        expect(logical_expr.operator_.type_ == oper);
        check_left(ast, logical_expr.left_);
        check_right(ast, logical_expr.right_);
    };
}

constexpr auto unary_expr(ctlox::token_type oper, auto check_right) {
    return [=](const auto& ast, ctlox::v2::flat_expr_ptr node_ptr) {
        const auto& unary_expr = expect_holds<ctlox::v2::flat_unary_expr>(ast, node_ptr);
        expect(unary_expr.operator_.type_ == oper);
        check_right(ast, unary_expr.right_);
    };
}

constexpr auto variable_expr(std::string_view name) {
    return [=](const auto& ast, ctlox::v2::flat_expr_ptr node_ptr) {
        const auto& variable_expr = expect_holds<ctlox::v2::flat_variable_expr>(ast, node_ptr);
        expect(variable_expr.name_.lexeme_ == name);
    };
}

constexpr auto print_stmt(auto check_expression) {
    return expression_stmt(call_expr(variable_expr("println"), check_expression));
}

constexpr bool test_ast(std::string_view source, auto... check_statements) {
    const ctlox::v2::flat_ast ast = ctlox::v2::serialize(ctlox::v2::parse(ctlox::v2::scan(source)));
    const auto& statements = ast.root_block_;

    expect(statements.size() == sizeof...(check_statements));

    auto it = statements.begin();
    (check_statements(ast, *it++), ...);

    expect(it == statements.end());

    return true;
}

// clang-format off
static_assert(test_ast("var foo;",
    var_stmt("foo", null_expr())
));

static_assert(test_ast("var foo = 15;",
    var_stmt("foo", literal_expr(15.0))
));

static_assert(test_ast(
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


static_assert(test_ast("!((((a)) * (b + c)) == 17);",
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

    const auto ast = ctlox::v2::serialize(ctlox::v2::parse(ctlox::v2::scan(source)));

    expect(ast.root_block_.first_.i == 0);
    expect(ast.root_block_.last_.i == 3);

    expect(ast.statements_.size() == 6);
    expect(ast.expressions_.size() == 20);

    return true;
}

static_assert(test_sizes());

namespace test_static_serialize {
    constexpr auto source = R"(
var foo = (12 + 13) / 2;
{
    print foo;
    var space = " ";
    foo = "Hello," + space + "world!";
}
print !foo;

while (nil or foo) {
    if (!foo)
        print "impossible!";
    else
        print "all is well";
    print "fooful";
}
)";

    constexpr ctlox::v2::ast_generator auto gen = [] { return ctlox::v2::parse(ctlox::v2::scan(source)); };
    constexpr auto ast = ctlox::v2::static_serialize<gen>();

    static_assert(ast.root_block_.first_.i == 0);
    static_assert(ast.root_block_.last_.i == 4);

    // clang-format off
    constexpr auto check_0 =
        var_stmt("foo",
            binary_expr(ctlox::token_type::slash,
                grouping_expr(
                    binary_expr(ctlox::token_type::plus,
                        literal_expr(12.0),
                        literal_expr(13.0))),
                literal_expr(2.0)));
    constexpr auto check_1 =
        block_stmt(
            print_stmt(
                variable_expr("foo")),
            var_stmt("space",
                literal_expr(" ")),
            expression_stmt(
                assign_expr("foo",
                    binary_expr(ctlox::token_type::plus,
                        binary_expr(ctlox::token_type::plus,
                            literal_expr("Hello,"),
                            variable_expr("space")),
                        literal_expr("world!"))))
        );
    constexpr auto check_2 =
        print_stmt(
            unary_expr(ctlox::token_type::bang,
                variable_expr("foo")));
    constexpr auto check_3 =
        while_stmt(
            logical_expr(ctlox::token_type::_or,
                literal_expr(ctlox::v2::nil),
                variable_expr("foo")),
            block_stmt(
                if_stmt(
                    unary_expr(ctlox::token_type::bang,
                        variable_expr("foo")),
                    print_stmt(literal_expr("impossible!")),
                    print_stmt(literal_expr("all is well"))),
                print_stmt(literal_expr("fooful"))
            ));

    static_assert((check_0(ast, ast.root_block_[0]), true));
    static_assert((check_1(ast, ast.root_block_[1]), true));
    static_assert((check_2(ast, ast.root_block_[2]), true));
    static_assert((check_3(ast, ast.root_block_[3]), true));
}  // namespace test_static_serialize

}  // namespace test_v2::test_serializer