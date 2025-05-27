#pragma once

#include <ctlox/v2/expression.hpp>
#include <ctlox/v2/statement.hpp>

#include <ctlox/v2/flat_ast.hpp>

// given a static_ast template parameter,
// generate a recursive tree of lambdas,
// which take a single argument of type program_state,
// and which return an evaluated value (if an expr).
// program_state contains an environment and a
// program output (vector<string> or vector<literal_t>).


namespace ctlox::v2 {

class code_generator {
public:
    auto operator()();

private:
};

auto generate_code();

}  // namespace ctlox::v2