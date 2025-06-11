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

fun make_counter(start) {
    var i = start;
    fun count() {
        var c = i;
        i = i + 1;
        return c;
    }
    return count;
}

var counter = make_counter(7);
print counter();
print counter();
print counter();

// print 1 > false;  // throws a ctlox::runtime_error
)"_lox;

static_assert(sizeof(program) == 1);

constexpr int count_prints() {
    int i = 0;
    auto on_print = [&i](const ctlox::v2::value_t&) { ++i; };
    program([&on_print](ctlox::v2::environment* env) { env->define_native<1>("println", on_print); });
    return i;
}

constexpr auto fibonacci = R"(
println("-----");
fun fib(n) {
    if (n > 1) return fib(n - 1) + fib(n - 2);
    return n;
}

var t0 = clock();
println(fib(30));
println("lox fibonacci took:");
println(clock() - t0);
)"_lox;

constexpr double fib_impl(double n) {
    if (n > 1)
        return fib_impl(n - 1) + fib_impl(n - 2);
    return n;
}

constexpr void fib_native() {
    using double_time_point_t = std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<double>>;

    std::println("-----");
    const double_time_point_t t0 = std::chrono::system_clock::now();
    std::println("{}", fib_impl(30));
    std::println("native fibonacci took: {}", double_time_point_t(std::chrono::system_clock::now()) - t0);
}

int main() try {
    constexpr int prints = count_prints();
    std::print("program prints {} times.\n\n", prints);

    program();

    fibonacci();
    fib_native();

    return 0;
} catch (const ctlox::v2::runtime_error& e) {
    std::print(std::cerr, "Error at line {} ({:?}): {}\n", e.token_.line_, e.token_.lexeme_, e.what());
    return 1;
}

}  // namespace ctlox_main::v2