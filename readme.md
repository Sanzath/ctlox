# ctlox

`ctlox` is Lox from [Crafting Interpreters](https://craftinginterpreters.com/) in C++, at compile-time.

## v1

Implemented using a pipeline-style alias template syntax, heavily inspired
by projects like [tmp](https://github.com/odinthenerd/tmp), with a sprinkling
of concepts from [Rappel](https://www.youtube.com/watch?v=itnyR9j8y6E) (currently closed-source).

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



This library distinguishes between functions with take exactly one argument
and those which take a pack of arguments.