# ctlox

`ctlox` is Lox from [Crafting Interpreters](https://craftinginterpreters.com/) in C++, at compile-time.

## ctlox::v2

Implemented using `constexpr` everywhere.

The advanced techniques employed here are heavily based upon the work by Jules Pénochot in
[Advanced techniques for high performance code generation](https://theses.hal.science/tel-04751989v1/document)
and its reference repository [poacher](https://github.com/JPenuchot/poacher/).

### Usage

A Lox program is compiled using the `ctlox::v2::compile<source>()` entrypoint,
or by using the `_lox` user-defined literal suffix. This program can then be invoked
as-is to execute the program, and may be invoked any number of times.

```c++
using namespace ctlox::v2::literals;
constexpr auto program = R"(
print "Hello, world!";
)"_lox;

program();  // writes "Hello, world!\n" to stdout
```

By default, program output is printed directly to stdout. This may be customized
by passing in a `print_fn` callable which accepts a `ctlox::v2::value_t`, which
will subsequently be invoked when each `print` statement is executed. This allows
the program to be executed in a constexpr context.

```c++
constexpr int count_prints() {
    int i = 0;
    program([&i](const ctlox::v2::value_t&) { ++i; });
    return i;
}
static_assert(count_prints() == 6);
```

### Details

The scanner and the parser are implemented mostly as-is from JLox, with some adjustments
where necessary to be constexpr-compatible.

Following that, as described in "Advanced techniques for high performance code generation",
the AST is serialized into a fixed-size flat tree, allowing it to be used as a non-type 
template parameter. This flat AST is passed as such to the code generator, which traverses 
the AST and generates a tree of embedded lambdas.

### Lox (v2)

The goal is to implement all of Lox in ctlox::v2. As of the time of writing, chapters 1-8
are implemented.

MSVC fails to compile some of the constexpr/template patterns employed here. I've managed
to find workarounds for some of the valid code, but

Some specific constexpr/template patterns fails to compile with MSVC. 

------

## ctlox::v1

Implemented using a pipeline-style alias template syntax, partially inspired
by projects like [tmp](https://github.com/odinthenerd/tmp), with a sprinkling
of concepts from [Rappel](https://www.youtube.com/watch?v=itnyR9j8y6E) (currently closed-source).

### Usage

A Lox program is parsed into an AST type using `ctlox::v1::compile<source>`.
This program may then be passed to `ctlox::v1::execute<program>`, which will
return a type-list containing the printed values.

```c++
using namespace ctlox::v1::literals;
using program = ctlox::v1::compile<R"(
print "Hello, world!";
)"_ct>;

using output = ctlox::v1::execute<program>;
static_assert(std::is_same_v<
    output, 
    ctlox::v1::list<ctlox::v1::value_t<"Hello, world!"_ct>>>);
```

### Details

At the core, `ctlox/v1/ct.hpp` contains a lot of scaffolding and a few generic functions.
The main user-facing syntaxes are to define pipelines:

```c++
// when invoked, my_pipeline replaces the first type in the list with int 
using my_pipeline = compose<
    drop<1>,
    prepend<int>>;
```

and to invoke pipelines:

```c++
using my_list = run<
    given_pack<char, double, int>,
    my_pipeline,
    listed>;
static_assert(std::is_same_v<my_list, list<int, double, int>>);
```

Like regular functions, alias functions are primarily defined by their inputs
and their outputs. Unlike regular functions, their outputs aren't returned to
the caller, but rather directly passed as the inputs to the next function in
the pipeline.

This library distinguishes between the two main types of inputs/outputs: exactly one type,
and a pack of types. The concepts `accepts_one` and `accepts_pack` represent
this distinction, and it is a compile-time error to chain a function which returns
a singular type into a function which accepts a pack, or vice versa. Internally, this
is checked with the above concepts and the usage of specific `call1` and `calln`
templates, so a function which "returns" a pack which happens to have exactly 1 type in it
will still properly be treated as a pack.

There is a special third kind of input: exactly zero types, modeled by `initiator`.
This kind of function may be used as the first item in a pipeline to provide types for
the next function in the pipeline. Specifically, `given_one<T>` and `given_pack<Ts...>`
are initiators which provide a singular type and a pack respectively.

There is also a special kind of output: functions which model `terminates_one`
or `terminates_pack` resolve to an actual type rather than passing arguments to another
function in the pipeline. Specifically, `returned` resolves to its singular argument `T`,
and `listed` resolves to `list<Ts...>`.

If a pipeline is executed and ends without having an explicit terminator, a special
function `deferred` is injected. This function models both `terminates_one` and `terminates_pack`,
and resolves to `given_one<T>` and `given_pack<Ts...>` respectively. This allows
the result of an unterminated pipeline to be used as the initiator to another pipeline:

```c++
using my_tuple_types = run<given_one<int>, repeat<2>, prepend<char>, append<double>>;
using my_tuple = run<my_tuple_types, applied<std::tuple>, returned>;
static_assert(std::is_same_v<my_tuple, std::tuple<char, int, int, double>>); 
```

A subset of Lox (chapters 1-8) is implemented in `ctlox::v1` as the three alias functions
`scan_ct`, `parse_ct` and `interpret_ct`.

* `scan_ct<s>` iterates over s and produces a pack of `token_t` specializations,
* `parse_ct` takes those tokens and produces a pack of `*_stmt` specializations,
  where their template arguments are those statements' substatements, and subexpressions,
* `interpret_ct` takes those statements and produces a pack of `value_t` specializations,
  each corresponding to an executed `print` statement and containing the resulting value,
  as-is (non-stringified).

Any error in this process, be it an invalid token, an invalid expression, or a "runtime" error,
results in a compile-time error.

The main limitation with this approach is that compilers begin to reject programs past a
relatively small level of complexity. For instance, adding a single subexpression to
the program in [`src/v1.cpp`](src/v1.cpp) causes MSVC to fail with error C1202, "recursive
type or function dependency context too complex". Hence, ctlox::v2.
