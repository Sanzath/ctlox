#include <ctlox/v2.hpp>

#include <iostream>
#include <print>

namespace ctlox_main::v2 {

using namespace ctlox::v2::literals;
constexpr auto program = R"(
var a = "foo";  // gets assigned in scope
var b = "bar";  // gets shadowed in scope
print a + " " + b;
{
    a = "Hello,";
    var b = "world!";
    print a + " " + b;
}
print a + " " + b;

if (1 > 2) print "zim"; else print "zam";

print 0.1 + 0.2;  // 0.30000000000000004 :)
print 1 > 2;
print nil or "zim" or false;

var str = "";
for (var i = 0; i < 100; i = i + 1) {
    print str;
    str = str + ".";
    if (i > 80) { break; }
}
print "foo";
// print 1 > false;  // throws a ctlox::runtime_error
)"_lox;

static_assert(sizeof(program) == 1);

constexpr int count_prints() {
    int i = 0;
    program([&i](const ctlox::v2::value_t&) { ++i; });
    return i;
}

int main() try {
    constexpr int prints = count_prints();
    std::print("program prints {} times.\n\n", prints);

    program();

    return 0;
} catch (const ctlox::v2::runtime_error& e) {
    std::print(std::cerr, "Error at line {} ({:?}): {}\n", e.token_.line_, e.token_.lexeme_, e.what());
    return 1;
}

}  // namespace ctlox_main::v2