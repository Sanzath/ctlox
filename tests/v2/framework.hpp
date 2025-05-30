#pragma once

#include <stdexcept>

namespace test_v2 {

// intentionally non-reference arguments to force values to be printed
// in the compiler output here (with clang-cl at least)
constexpr void fail_with(auto... debug_infos) { throw std::logic_error("test failed"); }

constexpr void expect(bool expectation_met, const auto&... debug_infos) {
    if (!expectation_met) {
        fail_with(debug_infos...);
    }
}

constexpr void expect_equal(const auto& left, const auto& right) {
    expect(left == right, "expected values to be equal: ", left, " and ", right);
}

}  // namespace test_v2