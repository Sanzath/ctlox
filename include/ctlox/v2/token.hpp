#pragma once

#include <ctlox/common/token_type.hpp>
#include <ctlox/v2/literal.hpp>

#include <format>
#include <string_view>

namespace ctlox::v2 {

struct token_t {
    token_type type_ = token_type::eof;
    std::string_view lexeme_;
    literal_t literal_;
    int line_ = -1;
};

}  // namespace ctlox::v2
