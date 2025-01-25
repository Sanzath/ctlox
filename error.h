#pragma once

#include <string>

namespace ctlox {
    struct error {
        int line_;
        std::string message_;
    };
}