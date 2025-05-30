#pragma once

#include <ctlox/v2/token.hpp>

#include <stdexcept>

namespace ctlox::v2 {

class scan_error : public std::invalid_argument {
public:
    scan_error(int line, const char* msg)
        : invalid_argument(msg)
        , line_(line) { }

    int line_ = -1;
};

class parse_error : public std::invalid_argument {
public:
    parse_error(const token_t& token, const char* msg)
        : invalid_argument(msg)
        , token_(token) { }

    token_t token_;
};

class runtime_error : public std::runtime_error {
public:
    runtime_error(const token_t& token, const char* msg)
        : std::runtime_error(msg)
        , token_(token) { }

    token_t token_;
};

}  // namespace ctlox::v2