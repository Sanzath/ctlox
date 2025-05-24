#include <algorithm>

#include <ctlox/v1/ctlox.hpp>

template <ctlox::string s>
using lox = ctlox::compose<
    ctlox::scan_ct<s>,
    ctlox::parse_ct,
    ctlox::interpret_ct>;

template <ctlox::string s>
using compile = ctlox::compose<
    ctlox::scan_ct<s>,
    ctlox::parse_ct>;

template <typename Ast>
using interpret = ctlox::run<Ast, ctlox::interpret_ct, ctlox::listed>;

int main()
{
    using ast = compile<R"(
var a = 15;
{
    a = a / 2;
    var a = "foo";
    print a;
}
print a;
)">;

    using output = interpret<ast>;

    using namespace ctlox::literals;
    static_assert(std::is_same_v<
        output,
        ctlox::list<
            ctlox::value_t<"foo"_ct>,
            ctlox::value_t<7.5>>>);

    return 0;
}
