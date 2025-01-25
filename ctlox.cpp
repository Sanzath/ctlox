#include <ranges>
#include <algorithm>
#include <array>
#include <iostream>
#include <variant>
#include <format>
#include <vector>
#include <charconv>

#include "scanner.h"
#include "scanner_ct.h"

namespace ctlox {
    template <std::size_t N>
    struct string_literal {
        constexpr string_literal() = default;

        constexpr string_literal(const char(&str)[N + 1]) {
            if constexpr (N != 0) {
                std::ranges::copy_n(str, N, str_.begin());
            }
        }

        constexpr string_literal(std::string_view str) {
            if constexpr (N != 0) {
                std::ranges::copy_n(str.begin(), N, str_.begin());
            }
        }

        constexpr auto size() const { return N; };
        constexpr bool empty() const { return N == 0; };

        constexpr auto begin() { return str_.begin(); };
        constexpr auto begin() const { return str_.begin(); };
        constexpr auto end() { return str_.end(); };
        constexpr auto end() const { return str_.end(); };

        constexpr char operator[](int idx) const { return str_[idx]; };

        constexpr operator std::string_view() const {
            return std::string_view(str_.begin(), str_.end());
        }

        constexpr auto operator<=>(const string_literal&) const = default;
        constexpr bool operator==(const string_literal&) const = default;

        std::array<char, N> str_ = {};
    };

    template <std::size_t N>
    string_literal(const char(&str)[N]) -> string_literal<N - 1>;


    template <typename expr_left, typename expr_right>
    struct binary_expr {
        expr_left left_;
        token operator_;
        expr_right right_;
    };

    template <typename expr>
    struct grouping_expr {
        expr expression_;
    };

    struct literal_expr {
        literal_t literal_;
    };

    template <typename expr>
    struct unary_expr {
        token operator_;
        expr right_;
    };

    struct program_output {
        std::string out_;
        std::string err_;
    };

    constexpr program_output run(std::string_view s) {
        scanner scanner(s);
        const auto scan_output = scanner.scan_tokens();

        program_output output{};
        for (const auto& token : scan_output.tokens_) {
            output.out_.append(token.to_string() + '\n');
        }

        for (const auto& error : scan_output.errors_) {
            auto message = std::string("[line ") + print_int(error.line_) + "] Error: " + error.message_ + '\n';
            output.err_.append(std::move(message));
        }

        return output;
    }

    struct program_output_sizes {
        std::size_t out_size_;
        std::size_t err_size_;
    };

    template <program_output_sizes sizes>
    struct program_output_ct {
        string_literal<sizes.out_size_> out_;
        string_literal<sizes.err_size_> err_;
    };

    template <string_literal s>
    struct run_ct_impl {
        static constexpr inline program_output_sizes sizes = [] {
            auto output = run(s);
            return program_output_sizes{
                .out_size_ = output.out_.size(),
                .err_size_ = output.err_.size(),
            };
            }();

        using program_output_type = program_output_ct<sizes>;

        static constexpr inline program_output_type program_output = [] {
            auto output = run(s);
            return program_output_type{
                .out_ = std::string_view(output.out_),
                .err_ = std::string_view(output.err_),
            };
            }();
    };

    template <string_literal s>
    constexpr inline auto run_ct = run_ct_impl<s>::program_output;
}

void test_run_ct() {
    //    constexpr auto output_ct = ctlox::run_ct<R"lox(
    //while (true) { class s {} };
    //var u = 123.45;
    //var v = "huh??"
    //var w = nil;
    //)lox">;
    //
    //    std::cerr << std::string_view(output_ct.err_) << '\n';
    //    std::cout << std::string_view(output_ct.out_) << '\n';
}

struct ast_printer {
    template <typename left_expr, typename right_expr>
    std::string operator()(const ctlox::binary_expr<left_expr, right_expr>& expr) {
        return std::format("({} {} {})", expr.operator_.lexeme_, (*this)(expr.left_), (*this)(expr.right_));
    }
    template <typename Expr>
    std::string operator()(const ctlox::unary_expr<Expr>& expr) {
        return std::format("({} {})", expr.operator_.lexeme_, (*this)(expr.right_));
    }
    template <typename Expr>
    std::string operator()(const ctlox::grouping_expr<Expr>& expr) {
        return std::format("(group {})", (*this)(expr.expression_));
    }
    std::string operator()(const ctlox::literal_expr& expr) {
        return std::visit(*this, expr.literal_);
    }
    std::string operator()(std::monostate) {
        return "nil";
    }
    std::string operator()(double v) {
        return ctlox::print_double(v);
    }
    std::string operator()(const std::string& s) {
        return s;
    }
};

void test_expr() {
    using namespace ctlox;
    auto expr =
        binary_expr{
            .left_ = unary_expr{
                .operator_ = token{.type_ = token_type::minus, .lexeme_ = "-", .line_ = 1},
                .right_ = literal_expr{123.0}
            },
            .operator_ = token{.type_ = token_type::star, .lexeme_ = "*", .line_ = 1},
            .right_ = grouping_expr{
                .expression_ = literal_expr{45.67}
            }
    };
    std::cout << ast_printer{}(expr) << '\n';

    constexpr literal_t value = 123.45;
}

int main()
{
    test_run_ct();
    test_expr();
}
