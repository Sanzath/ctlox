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

print 0.1 + 0.2;  // 0.30000000000000004 :)
print 1 > 2;
print nil;

// print 1 > false;  // throws a ctlox::runtime_error
)"_lox;

constexpr int count_prints() {
    int i = 0;
    program([&i](const ctlox::v2::value_t&) { ++i; });
    return i;
}
static_assert(count_prints() == 6);

int main() try {
    program();
    program();

    return 0;
} catch (const ctlox::v2::runtime_error& e) {
    std::print(std::cerr, "Error at line {} ({:?}): {}\n", e.token_.line_, e.token_.lexeme_, e.what());
    return 1;
}

}  // namespace ctlox_main::v2