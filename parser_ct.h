#pragma once

#include "ct.h"


namespace ctlox {
    struct parse_ct {
    private:
    public:
        using has_fn = void;
        template <accepts_one C, typename... Tokens>
        using fn = call1<C, void>;
    };

    namespace parse_tests {

    }
}