#include <ctlox/v1.hpp>

namespace ctlox_main::v1 {

// Changing line 23 to `a = a / 2 + 1` causes this program to fail to compile.
using output = ctlox::v1::compile_and_execute<R"(
var a = 15;
{
    a = a / 2;
    var a = "foo";
    print a;
}
print a;
)">;

using namespace ctlox::literals;
static_assert(std::is_same_v<
    output,
    ctlox::v1::list<
        ctlox::v1::value_t<"foo"_ct>,
        ctlox::v1::value_t<7.5>>>);

}
