
#include <ctlox/v1/interpreter.hpp>
#include <ctlox/v1/parser.hpp>
#include <ctlox/v1/scanner.hpp>

namespace test_v1::test_interpreter {
using namespace ctlox::common;
using namespace ctlox::v1;

// TODO: comprehensive env tests

static_assert(std::is_same_v<
    env::declare<
        env::env_t<
            env::entry_t<"foo", value_t<nil>>,
            env::entry_t<"bar", value_t<nil>>,
            env::entry_t<"baz", value_t<nil>>,
            env::entry_t<"quz", value_t<nil>>,
            env::entry_t<"quux", value_t<nil>>,
            env::end_of_environment>,
        "zaber", value_t<7.0>>,
    env::env_t<
        env::entry_t<"foo", value_t<nil>>,
        env::entry_t<"bar", value_t<nil>>,
        env::entry_t<"baz", value_t<nil>>,
        env::entry_t<"quz", value_t<nil>>,
        env::entry_t<"quux", value_t<nil>>,
        env::entry_t<"zaber", value_t<7.0>>,
        env::end_of_environment>>);

template <string s, auto _expected>
constexpr bool test()
{
    using program_output = run<
        scan_ct<s>,
        parse_ct,
        interpret_ct,
        at<0>,
        returned>;
    if constexpr (_expected == program_output::value) {
        return true;
    } else {
        using output = run<
            given_pack<value_t<_expected>, typename program_output::type>,
            errored>;
        return false;
    }
}

static_assert(concat("hello"_ct, ", "_ct, "world!"_ct) == "hello, world!"_ct);

constexpr bool r0 = test<"print 1;", 1.0>();
constexpr bool r1 = test<"print -5;", -5.0>();
constexpr bool r2 = test<R"(print !!!!"hello";)", true>();
constexpr bool r3 = test<R"(print (nil);)", nil>();
constexpr bool r4 = test<R"(print 4 <= 4;)", true>();
constexpr bool r5 = test<R"(print 4 < 3;)", false>();
constexpr bool r6 = test<"print 5 * 100 / 22;", 5.0 * 100.0 / 22.0>();
constexpr bool r7 = test<"print 5 * (100 / 22);", 5.0 * (100.0 / 22.0)>();
constexpr bool r8 = test<"var foo; var bar; foo = (bar = 2) + 5; print foo;", 7.0>();
constexpr bool r9 = test<"var foo; { var bar = 1; foo = bar; } print foo;", 1.0>();
}