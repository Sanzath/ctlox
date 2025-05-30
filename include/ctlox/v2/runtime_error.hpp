#pragma once

#include <ctlox/v2/token.hpp>

#include <stdexcept>

namespace ctlox::v2 {

class runtime_error : public std::runtime_error {
public:
    runtime_error(const token_t& token, const char* msg)
        : std::runtime_error(msg)
        , token_(token) { }

    token_t token_;
};

}  // namespace ctlox::v2