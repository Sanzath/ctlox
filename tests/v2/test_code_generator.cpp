#include <ctlox/v2/code_generator.hpp>

#include <ctlox/common/string.hpp>
#include <ctlox/v2/parser.hpp>
#include <ctlox/v2/scanner.hpp>
#include <ctlox/v2/serializer.hpp>

#include "framework.hpp"

namespace test_v2::test_code_generator {

template <ctlox::string source>
constexpr auto generate_code_for() {
    constexpr auto generate_ast = [] { return ctlox::v2::parse(ctlox::v2::scan(source)); };
    static constexpr auto ast = ctlox::v2::static_serialize<generate_ast>();
    return ctlox::v2::generate_code<ast>();
}

template <ctlox::string source>
constexpr bool test_program(std::initializer_list<ctlox::v2::value_t> expected_output) {
    auto program = generate_code_for<source>();
    std::vector<ctlox::v2::value_t> output;
    auto print_fn = [&output](ctlox::v2::value_t value) { output.push_back(std::move(value)); };
    program(print_fn);

    expect_equal(output.size(), expected_output.size());
    for (const auto& [value, expected_value] : std::views::zip(output, expected_output)) {
        expect_equal(value, expected_value);
    }

    return true;
}

using namespace std::string_literals;

static_assert(test_program<R"(
var a = 15;
{
    a = a / 2;
    var a = "foo";
    print a;
}
print a;
)">({ "foo"s, 7.5 }));

static_assert(test_program<R"(
var a = 1; var b = 2; var c = 3;
{ var a = 4; var b = 5;
  { var a = 6;
    print a; print b; print c; }
  print a; print b; print c; }
print a; print b; print c;
)">({ 6.0, 5.0, 3.0, 4.0, 5.0, 3.0, 1.0, 2.0, 3.0 }));

static_assert(test_program<R"(
var w = 1 > 2 == 2 < 1;
print w;
var x = (1 + 2) / 3 * 4 - -7;
print x;
var y = "foo" + "bar" + "baz";
print y;
var z = !!!(y == "foobarbaz" != true);
print z;
print nil;
)">({
    (1 > 2 == 2 < 1),
    ((1.0 + 2.0) / 3.0 * 4.0 - -7.0),
    "foobarbaz"s,
    true,
    ctlox::v2::nil,
}));

static_assert(test_program<R"(
var foo = 0;
var bar = 1;
if (foo > bar) print foo; else print bar;
foo = 10;
if (foo > bar) print foo; else print bar;
if (foo and bar and false) print "foo";
)">({ 1.0, 10.0 }));

// >256 statements in one block
static_assert(test_program<R"(
0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0;  // 32
0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0;  // 64
0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0;
0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0;  // 128
0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0;
0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0;
0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0;
0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0; 0;  // 256
0;                                                                                               // 257
)">({}));

static_assert(test_program<R"(
var b = 0;
while (b < 4) {
    print b;
    b = b + 1;
}
)">({ 0.0, 1.0, 2.0, 3.0 }));

static_assert(test_program<R"(
for (var b = 0; b < 4; b = b + 1) print b;
)">({ 0.0, 1.0, 2.0, 3.0 }));

static_assert(test_program<R"(
var a = 0;
var temp;

for (var b = 1; a < 5000; b = temp + b) {
  print a;
  temp = a;
  a = b;
}
)">({ 0., 1., 1., 2., 3., 5., 8., 13., 21., 34., 55., 89., 144., 233., 377., 610., 987., 1597., 2584., 4181. }));

}  // namespace test_v2::test_code_generator