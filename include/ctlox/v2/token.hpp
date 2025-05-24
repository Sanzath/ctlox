#pragma once

#include <ctlox/common/token_type.hpp>
#include <ctlox/v2/literal.hpp>

#include <format>
#include <string_view>
#include <string>

namespace ctlox::v2 {

struct token {
    token_type type_;
    std::string_view lexeme_;  // TODO: string_view or string?
    literal_t literal_;
    int line_;
};

}  // namespace ctlox::v2
