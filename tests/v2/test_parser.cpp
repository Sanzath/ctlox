#include "framework.hpp"

#include <ctlox/v2/parser.hpp>
#include <ctlox/v2/scanner.hpp>

#include <vector>

namespace test_v2::test_parser {

using namespace std::string_view_literals;

template <typename T>
constexpr const T& expect_holds(const auto& node_ptr) {
    expect(node_ptr != nullptr);

    if (const T* node = node_ptr->template get_if<T>()) {
        return *node;
    }

    throw std::logic_error("Node holds incorrect type");
}

constexpr auto null_stmt() {
    return [](const ctlox::v2::stmt_ptr& node_ptr) { expect(node_ptr == nullptr); };
}

constexpr auto block_stmt(auto... check_statements) {
    return [=](const ctlox::v2::stmt_ptr& node_ptr) {
        const auto& block_stmt = expect_holds<ctlox::v2::block_stmt>(node_ptr);
        const auto& statements = block_stmt.statements_;

        expect(statements.size() == sizeof...(check_statements));

        auto it = statements.begin();
        (check_statements(*it++), ...);

        expect(it == statements.end());
    };
}

constexpr auto expression_stmt(auto check_expression) {
    return [=](const ctlox::v2::stmt_ptr& node_ptr) {
        const auto& expression_stmt = expect_holds<ctlox::v2::expression_stmt>(node_ptr);
        check_expression(expression_stmt.expression_);
    };
}

constexpr auto if_stmt(auto check_condition, auto check_then_branch, auto check_else_branch) {
    return [=](const ctlox::v2::stmt_ptr& node_ptr) {
        const auto& if_stmt = expect_holds<ctlox::v2::if_stmt>(node_ptr);
        check_condition(if_stmt.condition_);
        check_then_branch(if_stmt.then_branch_);
        check_else_branch(if_stmt.else_branch_);
    };
}

constexpr auto var_stmt(std::string_view name, auto check_initializer) {
    return [=](const ctlox::v2::stmt_ptr& node_ptr) {
        const auto& var_stmt = expect_holds<ctlox::v2::var_stmt>(node_ptr);
        expect(var_stmt.name_.lexeme_ == name);
        check_initializer(var_stmt.initializer_);
    };
}

constexpr auto while_stmt(auto check_condition, auto check_body) {
    return [=](const ctlox::v2::stmt_ptr& node_ptr) {
        const auto& while_stmt = expect_holds<ctlox::v2::while_stmt>(node_ptr);
        check_condition(while_stmt.condition_);
        check_body(while_stmt.body_);
    };
}

constexpr auto null_expr() {
    return [](const ctlox::v2::expr_ptr& node_ptr) { expect(node_ptr == nullptr); };
}

constexpr auto assign_expr(std::string_view name, auto check_value) {
    return [=](const ctlox::v2::expr_ptr& node_ptr) {
        const auto& assign_expr = expect_holds<ctlox::v2::assign_expr>(node_ptr);
        expect(assign_expr.name_.lexeme_ == name);
        check_value(assign_expr.value_);
    };
}

constexpr auto binary_expr(ctlox::token_type oper, auto check_left, auto check_right) {
    return [=](const ctlox::v2::expr_ptr& node_ptr) {
        const auto& binary_expr = expect_holds<ctlox::v2::binary_expr>(node_ptr);
        expect(binary_expr.operator_.type_ == oper);
        check_left(binary_expr.left_);
        check_right(binary_expr.right_);
    };
}

constexpr auto call_expr(auto check_callee, auto... check_args) {
    return [=](const ctlox::v2::expr_ptr& node_ptr) {
        const auto& call_expr = expect_holds<ctlox::v2::call_expr>(node_ptr);

        check_callee(call_expr.callee_);

        auto it = call_expr.arguments_.begin();
        (check_args(*it++), ...);
    };
}

constexpr auto grouping_expr(auto check_expr) {
    return [=](const ctlox::v2::expr_ptr& node_ptr) {
        const auto& grouping_expr = expect_holds<ctlox::v2::grouping_expr>(node_ptr);
        check_expr(grouping_expr.expr_);
    };
}

constexpr auto literal_expr(const ctlox::v2::literal_t& literal) {
    return [=](const ctlox::v2::expr_ptr& node_ptr) {
        const auto& literal_expr = expect_holds<ctlox::v2::literal_expr>(node_ptr);
        expect(literal_expr.value_ == literal);
    };
}

constexpr auto logical_expr(ctlox::token_type oper, auto check_left, auto check_right) {
    return [=](const ctlox::v2::expr_ptr& node_ptr) {
        const auto& logical_expr = expect_holds<ctlox::v2::logical_expr>(node_ptr);
        expect(logical_expr.operator_.type_ == oper);
        check_left(logical_expr.left_);
        check_right(logical_expr.right_);
    };
}

constexpr auto unary_expr(ctlox::token_type oper, auto check_right) {
    return [=](const ctlox::v2::expr_ptr& node_ptr) {
        const auto& unary_expr = expect_holds<ctlox::v2::unary_expr>(node_ptr);
        expect(unary_expr.operator_.type_ == oper);
        check_right(unary_expr.right_);
    };
}

constexpr auto variable_expr(std::string_view name) {
    return [=](const ctlox::v2::expr_ptr& node_ptr) {
        const auto& variable_expr = expect_holds<ctlox::v2::variable_expr>(node_ptr);
        expect(variable_expr.name_.lexeme_ == name);
    };
}

constexpr auto print_stmt(auto check_expression) {
    return expression_stmt(call_expr(variable_expr("println"), check_expression));
}

constexpr bool test_statement(std::string_view source, auto check) {
    const auto statements = ctlox::v2::parse(ctlox::v2::scan(source));

    expect(statements.size() == 1);
    check(statements[0]);
    return true;
}

constexpr bool test_expression(std::string_view source, auto check) {
    const auto statements = ctlox::v2::parse(ctlox::v2::scan(source));

    expect(statements.size() == 1);
    const auto& statement = expect_holds<ctlox::v2::expression_stmt>(statements[0]);
    check(statement.expression_);
    return true;
}

namespace test_statements {
    // clang-format off
    static_assert(test_statement("123.52;",
        expression_stmt(
            literal_expr(123.52))));

    static_assert(test_statement("print nil;",
        print_stmt(
            literal_expr(ctlox::v2::nil))));

    static_assert(test_statement("var foo;",
        var_stmt("foo", null_expr())));

    static_assert(test_statement("var bar = true;",
        var_stmt("bar",
            literal_expr(true))));

    static_assert(test_statement(R"({ "zim"; false; })",
        block_stmt(
            expression_stmt(
                literal_expr("zim")),
            expression_stmt(
                literal_expr(false))
            )));

    static_assert(test_statement(R"(if (true) 1;)",
        if_stmt(
            literal_expr(true),
            expression_stmt(
                literal_expr(1.0)),
            null_stmt())));

    static_assert(test_statement(R"(if (true) 1; else 0;)",
        if_stmt(
            literal_expr(true),
            expression_stmt(
                literal_expr(1.0)),
            expression_stmt(
                literal_expr(0.0)))));

    // else binds to nearest if
    static_assert(test_statement(R"(if (true) if (false) 1; else 0;)",
        if_stmt(
            literal_expr(true),
            if_stmt(
                literal_expr(false),
                expression_stmt(literal_expr(1.0)),
                expression_stmt(literal_expr(0.0))),
            null_stmt())));

    static_assert(test_statement(R"(while (true) print "foo";)",
        while_stmt(
            literal_expr(true),
            print_stmt(literal_expr("foo")))));

    static_assert(test_statement(R"(for (;;) print "foo";)",
        while_stmt(
            literal_expr(true),
            print_stmt(literal_expr("foo")))));

    static_assert(test_statement(R"(for (; a < b;) print "foo";)",
        while_stmt(
            binary_expr(ctlox::token_type::less,
                variable_expr("a"),
                variable_expr("b")),
            print_stmt(literal_expr("foo")))));

    static_assert(test_statement(R"(for (; a < b; a = a + 1) print "foo";)",
        while_stmt(
            binary_expr(ctlox::token_type::less,
                variable_expr("a"),
                variable_expr("b")),
            block_stmt(
                print_stmt(
                    literal_expr("foo")),
                expression_stmt(
                    assign_expr("a",
                        binary_expr(ctlox::token_type::plus,
                            variable_expr("a"),
                            literal_expr(1.0))))
            ))));

    static_assert(test_statement(R"(for (a = 10;;) print "foo";)",
        block_stmt(
            expression_stmt(
                assign_expr("a",
                    literal_expr(10.0))),
            while_stmt(
                literal_expr(true),
                print_stmt(
                    literal_expr("foo")))
        )));

    static_assert(test_statement(R"(for (var a = 10;;) print "foo";)",
        block_stmt(
            var_stmt("a",
                literal_expr(10.0)),
            while_stmt(
                literal_expr(true),
                print_stmt(
                    literal_expr("foo")))
        )));
    // clang-format on
}  // namespace test_statements

namespace test_simple_expressions {
    // clang-format off
    static_assert(test_expression("foo = 7;",
        assign_expr("foo", literal_expr(7.0))));

    static_assert(test_expression("2 * 3;",
        binary_expr(ctlox::token_type::star, literal_expr(2.0), literal_expr(3.0))));

    static_assert(test_expression("2 / 3;",
        binary_expr(ctlox::token_type::slash, literal_expr(2.0), literal_expr(3.0))));

    static_assert(test_expression("2 + 3;",
        binary_expr(ctlox::token_type::plus, literal_expr(2.0), literal_expr(3.0))));

    static_assert(test_expression("2 - 3;",
        binary_expr(ctlox::token_type::minus, literal_expr(2.0), literal_expr(3.0))));

    static_assert(test_expression("2 > 3;",
        binary_expr(ctlox::token_type::greater, literal_expr(2.0), literal_expr(3.0))));

    static_assert(test_expression("2 >= 3;",
        binary_expr(ctlox::token_type::greater_equal, literal_expr(2.0), literal_expr(3.0))));

    static_assert(test_expression("2 < 3;",
        binary_expr(ctlox::token_type::less, literal_expr(2.0), literal_expr(3.0))));

    static_assert(test_expression("2 <= 3;",
        binary_expr(ctlox::token_type::less_equal, literal_expr(2.0), literal_expr(3.0))));

    static_assert(test_expression("2 == 3;",
        binary_expr(ctlox::token_type::equal_equal, literal_expr(2.0), literal_expr(3.0))));

    static_assert(test_expression("2 != 3;",
        binary_expr(ctlox::token_type::bang_equal, literal_expr(2.0), literal_expr(3.0))));

    static_assert(test_expression("(nil);",
        grouping_expr(literal_expr(ctlox::v2::nil))));

    static_assert(test_expression("!true;",
        unary_expr(ctlox::token_type::bang, literal_expr(true))));

    static_assert(test_expression("-2;",
        unary_expr(ctlox::token_type::minus, literal_expr(2.0))));

    static_assert(test_expression("baz;",
        variable_expr("baz")));
    // clang-format on
}  // namespace test_simple_expressions

namespace test_complex_expressions {
    // clang-format off: manually control how these tree formats are formatted

    // assignment binding order
    static_assert(test_expression("foo = bar = nil;",
        assign_expr("foo"sv,
            assign_expr("bar"sv,
                literal_expr(ctlox::v2::nil)))));

    // assignment/or precedence
    static_assert(test_expression("foo = bar or zim;",
        assign_expr("foo"sv,
            logical_expr(ctlox::token_type::_or,
                variable_expr("bar"sv),
                variable_expr("zim"sv)))));

    // or binding order
    static_assert(test_expression("foo or bar or zim;",
        logical_expr(ctlox::token_type::_or,
            logical_expr(ctlox::token_type::_or,
                variable_expr("foo"sv),
                variable_expr("bar"sv)),
            variable_expr("zim"sv))));

    // or/and precedence
    static_assert(test_expression("foo or bar and zim or baz and tok;",
        logical_expr(ctlox::token_type::_or,
            logical_expr(ctlox::token_type::_or,
                variable_expr("foo"sv),
                logical_expr(ctlox::token_type::_and,
                    variable_expr("bar"sv),
                    variable_expr("zim"sv))),
            logical_expr(ctlox::token_type::_and,
                variable_expr("baz"sv),
                variable_expr("tok"sv)))));

    // and binding order
    static_assert(test_expression("foo and bar and zim;",
        logical_expr(ctlox::token_type::_and,
            logical_expr(ctlox::token_type::_and,
                variable_expr("foo"sv),
                variable_expr("bar"sv)),
            variable_expr("zim"sv))));

    // and/equality precedence
    static_assert(test_expression("foo and bar == zim and baz;",
        logical_expr(ctlox::token_type::_and,
            logical_expr(ctlox::token_type::_and,
                variable_expr("foo"sv),
                binary_expr(ctlox::token_type::equal_equal,
                    variable_expr("bar"sv),
                    variable_expr("zim"sv))),
            variable_expr("baz"sv))));

    // equality binding order
    static_assert(test_expression("1 == 2 != 3 == 4;",
        binary_expr(ctlox::token_type::equal_equal,
            binary_expr(ctlox::token_type::bang_equal,
                binary_expr(ctlox::token_type::equal_equal,
                    literal_expr(1.0),
                    literal_expr(2.0)),
                literal_expr(3.0)),
            literal_expr(4.0))));

    // equality/comparison precedence
    static_assert(test_expression(R"("foo" > "bar" == "bar" <= zim;)",
        binary_expr(ctlox::token_type::equal_equal,
            binary_expr(ctlox::token_type::greater,
                literal_expr("foo"sv),
                literal_expr("bar"sv)),
            binary_expr(ctlox::token_type::less_equal,
                literal_expr("bar"sv),
                variable_expr("zim"sv)))));

    // comparison binding order
    static_assert(test_expression("1 < 2 >= 3 <= 4 > 5;",
        binary_expr(ctlox::token_type::greater,
            binary_expr(ctlox::token_type::less_equal,
                binary_expr(ctlox::token_type::greater_equal,
                    binary_expr(ctlox::token_type::less,
                        literal_expr(1.0),
                        literal_expr(2.0)),
                    literal_expr(3.0)),
                literal_expr(4.0)),
            literal_expr(5.0))));

    // comparison/term precedence
    static_assert(test_expression("1 > 2 + 3 < 4;",
        binary_expr(ctlox::token_type::less,
            binary_expr(ctlox::token_type::greater,
                literal_expr(1.0),
                binary_expr(ctlox::token_type::plus,
                    literal_expr(2.0),
                    literal_expr(3.0))),
            literal_expr(4.0))));

    // term binding order
    static_assert(test_expression(R"(foo + "bar" + baz - 3;)",
        binary_expr(ctlox::token_type::minus,
            binary_expr(ctlox::token_type::plus,
                binary_expr(ctlox::token_type::plus,
                    variable_expr("foo"sv),
                    literal_expr("bar"sv)),
                variable_expr("baz"sv)),
            literal_expr(3.0))));

    // term/factor precedence
    //        __(-)__
    //       /       \
    //    (+)         (/)
    //   /   \       /   \
    // (1)   (*)   (4)   (5)
    //      /   \
    //    (2)   (3)
    static_assert(test_expression("1 + 2 * 3 - 4 / 5;",
        binary_expr(ctlox::token_type::minus,
            binary_expr(ctlox::token_type::plus,
                literal_expr(1.0),
                binary_expr(ctlox::token_type::star,
                    literal_expr(2.0),
                    literal_expr(3.0))),
            binary_expr(ctlox::token_type::slash,
                literal_expr(4.0),
                literal_expr(5.0)))));

    // factor binding order
    static_assert(test_expression("1 / 2 / 3 * 4 / 5;",
        binary_expr(ctlox::token_type::slash,
            binary_expr(ctlox::token_type::star,
                binary_expr(ctlox::token_type::slash,
                    binary_expr(ctlox::token_type::slash,
                        literal_expr(1.0),
                        literal_expr(2.0)),
                    literal_expr(3.0)),
                literal_expr(4.0)),
            literal_expr(5.0))));

    // factor/unary precedence
    static_assert(test_expression("-2 * !false;",
        binary_expr(ctlox::token_type::star,
            unary_expr(ctlox::token_type::minus,
                literal_expr(2.0)),
            unary_expr(ctlox::token_type::bang,
                literal_expr(false)))));

    // unary binding order
    static_assert(test_expression("!!-val;",
        unary_expr(ctlox::token_type::bang,
            unary_expr(ctlox::token_type::bang,
                unary_expr(ctlox::token_type::minus,
                    variable_expr("val"))))));

    // grouping precedence
    static_assert(test_expression("-(a + b) * (c > d);",
        binary_expr(ctlox::token_type::star,
            unary_expr(ctlox::token_type::minus,
                grouping_expr(
                    binary_expr(ctlox::token_type::plus,
                        variable_expr("a"sv),
                        variable_expr("b"sv)))),
            grouping_expr(
                binary_expr(ctlox::token_type::greater,
                    variable_expr("c"sv),
                    variable_expr("d"sv))))));

    // grouping nesting
    static_assert(test_expression("!((((a)) * (b + c)) == 17);",
        unary_expr(ctlox::token_type::bang,
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
                    literal_expr(17.0))))));
    // clang-format on

}  // namespace test_complex_expressions

}  // namespace test_v2::test_parser