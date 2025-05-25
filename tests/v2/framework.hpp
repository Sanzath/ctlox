#pragma once

#include <stdexcept>

namespace test_v2 {

constexpr void expect(bool expectation_met) {
    if (!expectation_met) {
        throw std::logic_error("test failed");
    }
}

}