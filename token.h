#pragma once

#include <string>
#include <variant>
#include <format>

#include "token_type.h"
#include "numbers.h"

namespace ctlox {
    using literal_t = std::variant<std::monostate, std::string, double>;

    struct token {
        token_type type_;
        std::string lexeme_;
        literal_t literal_;
        int line_;

        constexpr std::string literal_to_string() const {
            return std::visit([](const auto& literal_value) -> std::string {
                if constexpr (std::is_same_v<decltype(literal_value), const double&>) {
                    return print_double(literal_value);
                }
                else if constexpr (std::is_same_v<decltype(literal_value), const std::string&>) {
                    return literal_value;
                }
                else {
                    return "";
                }
                }, literal_);
        }

        constexpr std::string to_string() const {
            if (std::is_constant_evaluated()) {
                return print_int(std::to_underlying(type_)) + " " + lexeme_ + " " + literal_to_string();
            }
            else {
                return std::format("{} {} {}", std::to_underlying(type_), lexeme_, literal_to_string());
            }
        }

        constexpr bool operator==(const token&) const = default;
    };
}
