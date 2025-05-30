#include <ctlox/v2/code_generator.hpp>

#include <ctlox/v2/parser.hpp>
#include <ctlox/v2/scanner.hpp>
#include <ctlox/v2/serializer.hpp>

#include "framework.hpp"

namespace test_v2::test_code_generator {
// TODO: cleanup

constexpr bool scratch_test() {
    constexpr auto source = R"(
var a = 15;
{
    a = a / 2;
    var a = "foo";
    print a;
}
print a;
)";
    constexpr auto make_ast = [&] { return ctlox::v2::parse(ctlox::v2::scan(source)); };
    static constexpr auto ast = ctlox::v2::static_serialize<make_ast>();

    auto program = ctlox::v2::generate_code<ast>();

    auto output = program();

    expect(output.size() == 2);

    using namespace std::string_literals;
    expect(output[0] == ctlox::v2::value_t("foo"s));
    expect(output[1] == ctlox::v2::value_t(7.5));

    return true;
}

constexpr bool depth_test() {
    constexpr auto source = R"(
var a = 1;
var b = 2;
var c = 3;
{
    var a = 4;
    var b = 5;
    {
        var a = 6;

        print a;
        print b;
        print c;
    }
    print a;
    print b;
    print c;
}
print a;
print b;
print c;
)";
    constexpr auto make_ast = [&] { return ctlox::v2::parse(ctlox::v2::scan(source)); };
    static constexpr auto ast = ctlox::v2::static_serialize<make_ast>();

    auto program = ctlox::v2::generate_code<ast>();

    auto output = program();

    expect(output.size() == 9);

    using namespace std::string_literals;
    expect(output[0] == ctlox::v2::value_t(6.0));
    expect(output[1] == ctlox::v2::value_t(5.0));
    expect(output[2] == ctlox::v2::value_t(3.0));

    expect(output[3] == ctlox::v2::value_t(4.0));
    expect(output[4] == ctlox::v2::value_t(5.0));
    expect(output[5] == ctlox::v2::value_t(3.0));

    expect(output[6] == ctlox::v2::value_t(1.0));
    expect(output[7] == ctlox::v2::value_t(2.0));
    expect(output[8] == ctlox::v2::value_t(3.0));

    return true;
}

constexpr bool op_test() {
    constexpr auto source = R"(
var x = (1 + 2) / 3 * 4 - -7;
print x;
var y = "foo" + "bar" + "baz";
print y;
var z = !!!(y == "foobarbaz" != true);
print z;
print nil;
)";
    constexpr auto make_ast = [&] { return ctlox::v2::parse(ctlox::v2::scan(source)); };
    static constexpr auto ast = ctlox::v2::static_serialize<make_ast>();

    auto program = ctlox::v2::generate_code<ast>();

    auto output = program();

    expect(output.size() == 4);

    using namespace std::string_literals;
    auto x = (1.0 + 2.0) / 3.0 * 4.0 - -7.0;
    auto y = "foobarbaz"s;
    auto z = true;
    expect(output[0] == ctlox::v2::value_t(x));
    expect(output[1] == ctlox::v2::value_t(y));
    expect(output[2] == ctlox::v2::value_t(z));
    expect(output[3] == ctlox::v2::value_t(ctlox::v2::nil));

    return true;
}

static_assert(scratch_test());
static_assert(depth_test());
static_assert(op_test());
}  // namespace test_v2::test_code_generator