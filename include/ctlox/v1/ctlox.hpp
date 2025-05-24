#pragma once

#include <ctlox/v1/ct.hpp>
#include <ctlox/v1/interpreter.hpp>
#include <ctlox/v1/numbers.hpp>
#include <ctlox/v1/parser.hpp>
#include <ctlox/v1/scanner.hpp>
#include <ctlox/v1/string.hpp>
#include <ctlox/v1/types.hpp>

namespace ctlox::v1 {
template <string s>
using compile = run<
    scan_ct<s>,
    parse_ct>;

template <typename Compiled>
using execute = run<
    Compiled,
    interpret_ct,
    listed>;

template <string s>
using compile_and_execute = run<
    scan_ct<s>,
    parse_ct,
    interpret_ct,
    listed>;
}
