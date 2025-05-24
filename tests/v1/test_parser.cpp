#include <ctlox/v1/scanner.hpp>
#include <ctlox/v1/parser.hpp>

using namespace ctlox::v1;

namespace test_parser {

static_assert(none == none);
static_assert(none != 154.0);

template <string s, typename... Statements>
constexpr inline bool test = std::same_as<
    list<Statements...>,
    run<scan_ct<s>, parse_ct, listed>>;

template <string s, typename... Statements>
using debugtest = run<scan_ct<s>, parse_ct, errored>;

using t0 = run<
    given_pack<
        token_t<0, token_type::number, "23.52", 123.52>,
        token_t<5, token_type::semicolon, ";">,
        token_t<6, token_type::eof, "">>,
    parse_ct,
    at<0>,
    returned>;
static_assert(std::same_as<t0, expression_stmt<literal_expr<123.52>>>);

static_assert(test<
    "(123.52);",
    expression_stmt<grouping_expr<literal_expr<123.52>>>>);
static_assert(test<
    "!123.52;",
    expression_stmt<unary_expr<token_t<0, token_type::bang, "!">, literal_expr<123.52>>>>);
static_assert(test<
    "-false;",
    expression_stmt<unary_expr<token_t<0, token_type::minus, "-">, literal_expr<false>>>>);
static_assert(test<
    "!!true;",
    expression_stmt<unary_expr<token_t<0, token_type::bang, "!">, unary_expr<token_t<1, token_type::bang, "!">, literal_expr<true>>>>>);
static_assert(test<
    "2 * 3;",
    expression_stmt<
        binary_expr<
            literal_expr<2.0>,
            token_t<2, token_type::star, "*">,
            literal_expr<3.0>>>>);
static_assert(test<
    "1 * -2 / !false;",
    expression_stmt<
        binary_expr<
            binary_expr<
                literal_expr<1.0>,
                token_t<2, token_type::star, "*">,
                unary_expr<token_t<4, token_type::minus, "-">, literal_expr<2.0>>>,
            token_t<7, token_type::slash, "/">,
            unary_expr<token_t<9, token_type::bang, "!">, literal_expr<false>>>>>);
static_assert(test<
    R"(13.3 + "stuff";)",
    expression_stmt<
        binary_expr<
            literal_expr<13.3>,
            token_t<5, token_type::plus, "+">,
            literal_expr<"stuff"_ct>>>>);
static_assert(test<
    R"(nil - nil - nil;)",
    expression_stmt<
        binary_expr<
            binary_expr<
                literal_expr<nil>,
                token_t<4, token_type::minus, "-">,
                literal_expr<nil>>,
            token_t<10, token_type::minus, "-">,
            literal_expr<nil>>>>);
static_assert(test<
    "1 + 2 * 3 - 4;",
    expression_stmt<
        binary_expr<
            binary_expr<
                literal_expr<1.0>,
                token_t<2, token_type::plus, "+">,
                binary_expr<
                    literal_expr<2.0>,
                    token_t<6, token_type::star, "*">,
                    literal_expr<3.0>>>,
            token_t<10, token_type::minus, "-">,
            literal_expr<4.0>>>>);
static_assert(test<"1 * 2 + 3 / 4;",
    expression_stmt<
        binary_expr<
            binary_expr<
                literal_expr<1.0>,
                token_t<2, token_type::star, "*">,
                literal_expr<2.0>>,
            token_t<6, token_type::plus, "+">,
            binary_expr<
                literal_expr<3.0>,
                token_t<10, token_type::slash, "/">,
                literal_expr<4.0>>>>>);
static_assert(test<"true > nil;",
    expression_stmt<
        binary_expr<literal_expr<true>, token_t<5, token_type::greater, ">">, literal_expr<nil>>>>);
static_assert(test<"1 > 2 <= 3 >= 0;",
    expression_stmt<
        binary_expr<
            binary_expr<
                binary_expr<
                    literal_expr<1.0>,
                    token_t<2, token_type::greater, ">">,
                    literal_expr<2.0>>,
                token_t<6, token_type::less_equal, "<=">,
                literal_expr<3.0>>,
            token_t<11, token_type::greater_equal, ">=">,
            literal_expr<0.0>>>>);
static_assert(test<"nil != nil == false;",
    expression_stmt<
        binary_expr<
            binary_expr<
                literal_expr<nil>,
                token_t<4, token_type::bang_equal, "!=">,
                literal_expr<nil>>,
            token_t<11, token_type::equal_equal, "==">,
            literal_expr<false>>>>);
static_assert(test<"7 == 2 * 3.5;",
    expression_stmt<
        binary_expr<
            literal_expr<7.0>,
            token_t<2, token_type::equal_equal, "==">,
            binary_expr<
                literal_expr<2.0>,
                token_t<7, token_type::star, "*">,
                literal_expr<3.5>>>>>);
static_assert(test<"(7 == 2) * 3.5;"_ct,
    expression_stmt<
        binary_expr<
            grouping_expr<
                binary_expr<
                    literal_expr<7.0>,
                    token_t<3, token_type::equal_equal, "==">,
                    literal_expr<2.0>>>,
            token_t<9, token_type::star, "*">,
            literal_expr<3.5>>>>);
static_assert(test<R"(1; print ("foo");)",
    expression_stmt<literal_expr<1.0>>,
    print_stmt<grouping_expr<literal_expr<"foo"_ct>>>>);
static_assert(test<"var foo; var bar = foo;",
    var_stmt<
        token_t<4, token_type::identifier, "foo">,
        literal_expr<nil>>,
    var_stmt<
        token_t<13, token_type::identifier, "bar">,
        variable_expr<token_t<19, token_type::identifier, "foo">>>>);
static_assert(test<
    "var foo; var bar; foo = (bar = 2) + 2;",
    var_stmt<
        token_t<4, token_type::identifier, "foo">,
        literal_expr<nil>>,
    var_stmt<
        token_t<13, token_type::identifier, "bar">,
        literal_expr<nil>>,
    expression_stmt<
        assign_expr<
            token_t<18, token_type::identifier, "foo">,
            binary_expr<
                grouping_expr<
                    assign_expr<
                        token_t<25, token_type::identifier, "bar">,
                        literal_expr<2.0>>>,
                token_t<34, token_type::plus, "+">,
                literal_expr<2.0>>>>>);
static_assert(test<
    "var foo; { var bar = 1; foo = bar; } print foo;",
    var_stmt<
        token_t<4, token_type::identifier, "foo">,
        literal_expr<nil>>,
    block_stmt<
        var_stmt<
            token_t<15, token_type::identifier, "bar">,
            literal_expr<1.0>>,
        expression_stmt<
            assign_expr<
                token_t<24, token_type::identifier, "foo">,
                variable_expr<token_t<30, token_type::identifier, "bar">>>>>,
    print_stmt<variable_expr<token_t<43, token_type::identifier, "foo">>>>);
}
