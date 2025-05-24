#pragma once

#include <string_view>
#include <string>
#include <algorithm>

namespace ctlox {
    constexpr double parse_double(std::string_view text) {
        auto dot = text.find('.');
        auto decimal_part = text.substr(0, dot);

        double value = 0.0;
        for (char c : decimal_part) {
            int i = c - '0';
            value = (value * 10) + i;
        }

        if (dot != std::string_view::npos) {
            auto fractional_part = text.substr(dot + 1);
            double divisor = 10;
            for (char c : fractional_part) {
                int i = c - '0';
                value = value + i / divisor;
                divisor *= 10;
            }
        }

        return value;
    }

    constexpr std::string print_int(int value) {
        std::string text;

        if (value == 0) {
            return "0";
        }

        for (; value != 0; value /= 10) {
            text.push_back((value % 10) + '0');
        }

        std::ranges::reverse(text);
        return text;
    }

    constexpr std::string print_double(double value) {
        auto whole = static_cast<std::int64_t>(value);
        value -= whole;
        for (int i = 0; i < 5; ++i) {
            value *= 10;
        }
        auto frac = static_cast<std::int64_t>(value);

        std::string rtext;

        auto append = [&rtext](auto V) {
            if (V != 0) {
                for (auto n = V; n; n /= 10)
                    rtext.push_back((V < 0 ? -1 : 1) * (n % 10) + '0');
            }
            else {
                rtext.push_back('0');
            }
            };

        append(frac);
        rtext.push_back('.');
        append(whole);
        std::ranges::reverse(rtext);
        while (rtext.ends_with('0')) {
            rtext.erase(rtext.end() - 1);
        }
        if (rtext.ends_with('.')) {
            rtext.erase(rtext.end() - 1);
        }
        return rtext;
    }

}