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
    static_assert(test_statement("123.52;", [](const ctlox::v2::stmt_ptr& node_ptr) {
        const auto& statement = expect_holds<ctlox::v2::expression_stmt>(node_ptr);

        const auto& expression = expect_holds<ctlox::v2::literal_expr>(statement.expression_);
        expect(expression.value_ == ctlox::v2::literal_t(123.52));
    }));

    static_assert(test_statement("print nil;", [](const ctlox::v2::stmt_ptr& node_ptr) {
        const auto& statement = expect_holds<ctlox::v2::print_stmt>(node_ptr);

        const auto& expression = expect_holds<ctlox::v2::literal_expr>(statement.expression_);
        expect(expression.value_ == ctlox::v2::literal_t(ctlox::v2::nil));
    }));

    static_assert(test_statement("var foo;", [](const ctlox::v2::stmt_ptr& node_ptr) {
        const auto& statement = expect_holds<ctlox::v2::var_stmt>(node_ptr);
        expect(statement.name_.lexeme_ == "foo"sv);
        expect(statement.initializer_ == nullptr);
    }));

    static_assert(test_statement("var bar = true;", [](const ctlox::v2::stmt_ptr& node_ptr) {
        const auto& statement = expect_holds<ctlox::v2::var_stmt>(node_ptr);
        expect(statement.name_.lexeme_ == "bar"sv);

        const auto& initializer = expect_holds<ctlox::v2::literal_expr>(statement.initializer_);
        expect(initializer.value_ == ctlox::v2::literal_t(true));
    }));

    static_assert(test_statement(R"({ "zim"; false; })", [](const ctlox::v2::stmt_ptr& node_ptr) {
        const auto& statement = expect_holds<ctlox::v2::block_stmt>(node_ptr);

        {
            const auto& sub_statement = expect_holds<ctlox::v2::expression_stmt>(statement.statements_[0]);

            const auto& expression = expect_holds<ctlox::v2::literal_expr>(sub_statement.expression_);
            expect(expression.value_ == ctlox::v2::literal_t("zim"));
        }

        {
            const auto& sub_statement = expect_holds<ctlox::v2::expression_stmt>(statement.statements_[1]);

            const auto& expression = expect_holds<ctlox::v2::literal_expr>(sub_statement.expression_);
            expect(expression.value_ == ctlox::v2::literal_t(false));
        }
    }));
}  // namespace test_statements

namespace test_simple_expressions {
    static_assert(test_expression("foo = 7;", [](const ctlox::v2::expr_ptr& node_ptr) {
        const auto& expression = expect_holds<ctlox::v2::assign_expr>(node_ptr);
        expect(expression.name_.lexeme_ == "foo"sv);

        const auto& value = expect_holds<ctlox::v2::literal_expr>(expression.value_);
        expect(value.value_ == ctlox::v2::literal_t(7.0));
    }));

    static_assert(test_expression("2 * 3;", [](const ctlox::v2::expr_ptr& node_ptr) {
        const auto& expression = expect_holds<ctlox::v2::binary_expr>(node_ptr);
        expect(expression.operator_.type_ == ctlox::token_type::star);

        const auto& left = expect_holds<ctlox::v2::literal_expr>(expression.left_);
        expect(left.value_ == ctlox::v2::literal_t(2.0));

        const auto& right = expect_holds<ctlox::v2::literal_expr>(expression.right_);
        expect(right.value_ == ctlox::v2::literal_t(3.0));
    }));

    static_assert(test_expression("2 / 3;", [](const ctlox::v2::expr_ptr& node_ptr) {
        const auto& expression = expect_holds<ctlox::v2::binary_expr>(node_ptr);
        expect(expression.operator_.type_ == ctlox::token_type::slash);
    }));

    static_assert(test_expression("2 + 3;", [](const ctlox::v2::expr_ptr& node_ptr) {
        const auto& expression = expect_holds<ctlox::v2::binary_expr>(node_ptr);
        expect(expression.operator_.type_ == ctlox::token_type::plus);
    }));

    static_assert(test_expression("2 - 3;", [](const ctlox::v2::expr_ptr& node_ptr) {
        const auto& expression = expect_holds<ctlox::v2::binary_expr>(node_ptr);
        expect(expression.operator_.type_ == ctlox::token_type::minus);
    }));

    static_assert(test_expression("2 > 3;", [](const ctlox::v2::expr_ptr& node_ptr) {
        const auto& expression = expect_holds<ctlox::v2::binary_expr>(node_ptr);
        expect(expression.operator_.type_ == ctlox::token_type::greater);
    }));

    static_assert(test_expression("2 >= 3;", [](const ctlox::v2::expr_ptr& node_ptr) {
        const auto& expression = expect_holds<ctlox::v2::binary_expr>(node_ptr);
        expect(expression.operator_.type_ == ctlox::token_type::greater_equal);
    }));

    static_assert(test_expression("2 < 3;", [](const ctlox::v2::expr_ptr& node_ptr) {
        const auto& expression = expect_holds<ctlox::v2::binary_expr>(node_ptr);
        expect(expression.operator_.type_ == ctlox::token_type::less);
    }));

    static_assert(test_expression("2 <= 3;", [](const ctlox::v2::expr_ptr& node_ptr) {
        const auto& expression = expect_holds<ctlox::v2::binary_expr>(node_ptr);
        expect(expression.operator_.type_ == ctlox::token_type::less_equal);
    }));

    static_assert(test_expression("2 == 3;", [](const ctlox::v2::expr_ptr& node_ptr) {
        const auto& expression = expect_holds<ctlox::v2::binary_expr>(node_ptr);
        expect(expression.operator_.type_ == ctlox::token_type::equal_equal);
    }));

    static_assert(test_expression("2 != 3;", [](const ctlox::v2::expr_ptr& node_ptr) {
        const auto& expression = expect_holds<ctlox::v2::binary_expr>(node_ptr);
        expect(expression.operator_.type_ == ctlox::token_type::bang_equal);
    }));

    static_assert(test_expression("(nil);", [](const ctlox::v2::expr_ptr& node_ptr) {
        const auto& expression = expect_holds<ctlox::v2::grouping_expr>(node_ptr);

        const auto& sub_expression = expect_holds<ctlox::v2::literal_expr>(expression.expr_);
        expect(sub_expression.value_ == ctlox::v2::literal_t(ctlox::v2::nil));
    }));

    static_assert(test_expression("!true;", [](const ctlox::v2::expr_ptr& node_ptr) {
        const auto& expression = expect_holds<ctlox::v2::unary_expr>(node_ptr);
        expect(expression.operator_.type_ == ctlox::token_type::bang);

        const auto& right = expect_holds<ctlox::v2::literal_expr>(expression.right_);
        expect(right.value_ == ctlox::v2::literal_t(true));
    }));

    static_assert(test_expression("-2;", [](const ctlox::v2::expr_ptr& node_ptr) {
        const auto& expression = expect_holds<ctlox::v2::unary_expr>(node_ptr);
        expect(expression.operator_.type_ == ctlox::token_type::minus);
    }));

    static_assert(test_expression("baz;", [](const ctlox::v2::expr_ptr& node_ptr) {
        const auto& expression = expect_holds<ctlox::v2::variable_expr>(node_ptr);
        expect(expression.name_.lexeme_ == "baz"sv);
    }));
}  // namespace test_simple_expressions

namespace test_complex_expressions {
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

    // clang-format off: manually control how these tree formats are formatted
    // assignment binding order
    static_assert(test_expression("foo = bar = nil;",
        assign_expr("foo"sv,
            assign_expr("bar"sv,
                literal_expr(ctlox::v2::nil)))));

    // assignment/equality precedence
    static_assert(test_expression("foo = bar == zim;",
        assign_expr("foo"sv,
            binary_expr(
                ctlox::token_type::equal_equal,
                variable_expr("bar"sv),
                variable_expr("zim"sv)))));

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