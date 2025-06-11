#pragma once

#include <ctlox/common/token_type.hpp>
#include <ctlox/v2/literal.hpp>
#include <ctlox/v2/flat_ptr.hpp>

#include <vector>
#include <string_view>

namespace ctlox::v2 {

struct token_t {
    token_type type_ = token_type::eof;
    std::string_view lexeme_;
    literal_t literal_;
    int line_ = -1;
};

using token_list = std::vector<token_t>;

using flat_token_ptr = flat_ptr<token_t>;
using flat_token_list = flat_list<token_t>;

}  // namespace ctlox::v2
