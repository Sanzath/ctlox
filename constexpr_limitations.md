While writing this, MSVC failed to compile at certain points, and I had to
make adjustments to compile the code. These are the problems I encountered
and the workarounds I implemented. They all seem related to "recursion" in
some way.

### `literal_t` `nil` assignment

In `ctlox::v2::scanner::identifier()`, the assignment `literal = nil;`
is not considered a constant expression. MSVC appears to believe it is
reading uninitialized memory.

Transforming the RHS into a `literal_t` before the assignment resolves the
problem.

### `stmt_t`/`expr_t` destructors

Both `stmt_t` and `expr_t` are defined to be self-referential, and both in
the same way. `expr_t` contains a variant of the various `*_expr` types,
and those types contain `unique_ptr<expr_t>` members. This seems to cause
both MSVC and LLVM to not recognize the default destructor at `constexpr`,
and so destroying an `expr_t` does not result in a constant evaluation.

Explicitly declaring the destructor and marking it as `constexpr` resolves
the problem.

### recursive functions with multiple returns

Some of `ctlox::v2::parser`'s member functions are defined to have multiple
return statements, namely `assignment()` and `unary()`. When one of these
function is called and recurses, the second call considers the `unique_ptr`
move-constructor to be a non-constant evaluation, due to a supposed read of
an uninitialized symbol.

Rewriting the function to have a single return statement resolves the problem.

### `std::vector` iteration

In `ctlox::v2::flattener`, certain statements involving vector iteration
failed to compile due to the checks in vector iterator seemingly accessing
an undefined symbol.

Constructing a span from the vector and iterating over that span resolves 
the problem.

### variable template parameter `const auto&`

When a variable template contains a `const auto&` non-type template parameter,
MSVC may fail to compile when it is used in certain contexts. The error message
claims that the template parameter was `const T* const&` and produces errors
based on that, despite actually passing `T const&`. This suggests a compiler bug,
reinforced by the fact that tweaking the code somewhat can produce an Internal
Compiler Error instead.

Namely, this error was encountered when implementing `ctlox::v2::static_visit_v`.

```c++
template <const auto& v>
constexpr const auto& foo() noexcept {
    return v.bar;
}

template <const auto& v>
constexpr const auto& foo_v = foo<v>();
```

It is sometimes possible to avoid this error by separating the template parameter
from the function call, or by not using the variable template entirely.

```c++
template <const auto& v>
struct foo_t { static constexpr const auto& foo() noexcept {
    return v.bar;
} };

template <const auto& v>
constexpr const auto& foo_v = foo_t<v>::foo();  // sometimes OK
```

### recursion in `<const T& value>` templates

In `ctlox::v2::code_generator`, when a block statement contains another block statement,
MSVC is unable to generate code due to the recursion, and generates the following error:

```
C:\_\Code\C++2\ctlox\include\ctlox/v2/code_generator.hpp(140): error C3779: 'ctlox::v2::code_generator<ctlox::v2::static_ast<17,15> const `bool __cdecl test_v2::test_code_generator::depth_test(void)'::`2'::ast>::generate_block': a function that returns 'auto' cannot be used before it is defined
```

I'm unconvinced that the error is valid, since these would be two different instantiations of
`generate_ast`, each with a distinct `flat_stmt_list` non-type template parameter.

It might be possible to rework the implementation to avoid the recursion. However, at this point,
this error has caused me to switch to LLVM clang-cl entirely.