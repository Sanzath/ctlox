#pragma once

#include <ctlox/v2/environment.hpp>
#include <ctlox/v2/exception.hpp>
#include <ctlox/v2/expression.hpp>
#include <ctlox/v2/flat_ast.hpp>
#include <ctlox/v2/statement.hpp>
#include <ctlox/v2/static_visit.hpp>
#include <ctlox/v2/types.hpp>

#include <functional>

namespace ctlox::v2 {

struct program_state {
    environment env_;
    std::vector<value_t>& output_;
};

struct _code_generator_base {
    static constexpr bool is_truthy(const value_t& value) {
        if (std::holds_alternative<nil_t>(value)) {
            return false;
        }
        if (const bool* b = std::get_if<bool>(&value)) {
            return *b;
        }
        return true;
    }

    template <const token_t& oper>
    static constexpr double& check_number_operand(value_t& value) {
        if (double* number = std::get_if<double>(&value)) {
            return *number;
        }

        throw runtime_error(oper, "Operand must be a number.");
    }

    template <const token_t& oper>
    static constexpr std::pair<double&, double&> check_number_operands(value_t& lhs, value_t& rhs) {
        double* lhs_number = std::get_if<double>(&lhs);
        double* rhs_number = std::get_if<double>(&rhs);

        if (lhs_number && rhs_number) {
            return { *lhs_number, *rhs_number };
        }

        throw runtime_error(oper, "Operands must be numbers.");
    }

    template <token_type type>
    static constexpr auto number_op_for() {
        if constexpr (type == token_type::less)
            return std::less {};
        else if constexpr (type == token_type::less_equal)
            return std::less_equal {};
        else if constexpr (type == token_type::greater)
            return std::greater {};
        else if constexpr (type == token_type::greater_equal)
            return std::greater_equal {};
        else if constexpr (type == token_type::minus)
            return std::minus {};
        else if constexpr (type == token_type::slash)
            return std::divides {};
        else if constexpr (type == token_type::star)
            return std::multiplies {};
        else
            return none;
    }

    template <token_type type>
    static constexpr auto value_op_for() {
        if constexpr (type == token_type::equal_equal)
            return std::equal_to {};
        else if constexpr (type == token_type::bang_equal)
            return std::not_equal_to {};
        else
            return none;
    }
};

template <const auto& ast>
    requires flat_ast_c<decltype(ast)>
struct code_generator : _code_generator_base {
    static constexpr auto generate() {
        auto root_block = generate_block<ast.root_block_>();
        return [=] -> std::vector<value_t> {
            std::vector<value_t> output;
            program_state state { .output_ = output };
            root_block(state);
            return output;
        };
    }

private:
    template <const flat_stmt_list& stmts>
    static constexpr auto generate_block() {
        constexpr auto index_sequence = std::make_index_sequence<stmts.size()> {};

        // create a tuple from the statements
        auto block_tuple = []<std::size_t... I>(std::index_sequence<I...>) {
            return std::tuple { visit<stmts[I]>()... };
        }(index_sequence);

        return [=](program_state& state) -> void {
            // call each statement in that tuple
            std::apply([&state](auto&&... stmt) { (stmt(state), ...); }, block_tuple);
        };
    }

    template <flat_stmt_ptr ptr>
    static constexpr auto visit() {
        return generate_stmt<static_visit<ast[ptr]>::value>();
    }

    template <flat_expr_ptr ptr>
    static constexpr auto visit() {
        return generate_expr<static_visit<ast[ptr]>::value>();
    }

    template <const flat_block_stmt& stmt>
    static constexpr auto generate_stmt() {
        auto block = generate_block<stmt.statements_>();
        return [=](program_state& state) -> void {
            program_state block_state { .env_ { &state.env_ }, .output_ = state.output_ };
            block(block_state);
        };
    }

    template <const flat_expression_stmt& stmt>
    static constexpr auto generate_stmt() {
        auto expr = visit<stmt.expression_>();
        return [=](program_state& state) -> void { static_cast<void>(expr(state)); };
    }

    template <const flat_print_stmt& stmt>
    static constexpr auto generate_stmt() {
        auto expr = visit<stmt.expression_>();
        return [=](program_state& state) -> void { state.output_.push_back(expr(state)); };
    }

    template <const flat_var_stmt& stmt>
    static constexpr auto generate_stmt() {
        if constexpr (stmt.initializer_ != flat_nullptr) {
            auto expr = visit<stmt.initializer_>();
            return [=](program_state& state) -> void { state.env_.define(stmt.name_, expr(state)); };
        } else {
            return [=](program_state& state) -> void { state.env_.define(stmt.name_, nil); };
        }
    }

    template <const flat_assign_expr& expr>
    static constexpr auto generate_expr() {
        auto right = visit<expr.value_>();
        return [=](program_state& state) -> value_t {
            auto value = right(state);
            state.env_.assign(expr.name_, value);
            return value;
        };
    }

    template <const flat_binary_expr& expr>
    static constexpr auto generate_expr() {
        auto left = visit<expr.left_>();
        auto right = visit<expr.right_>();

        constexpr token_type type = expr.operator_.type_;
        constexpr auto number_op = number_op_for<type>();
        constexpr auto value_op = value_op_for<type>();

        if constexpr (number_op != none) {
            return [=](program_state& state) -> value_t {
                value_t lhs = left(state);
                value_t rhs = right(state);
                auto [lhs_number, rhs_number] = check_number_operands<expr.operator_>(lhs, rhs);
                return number_op(lhs_number, rhs_number);
            };
        }

        else if constexpr (value_op != none) {
            return [=](program_state& state) -> value_t {
                value_t lhs = left(state);
                value_t rhs = right(state);
                return value_op(lhs, rhs);
            };
        }

        else if constexpr (type == token_type::plus) {
            return [=](program_state& state) -> value_t {
                value_t lhs = left(state);
                value_t rhs = right(state);

                return std::visit(
                    []<typename Left, typename Right>(const Left& lhs, const Right& rhs) -> value_t {
                        if constexpr (std::is_same_v<Left, std::string> && std::is_same_v<Right, std::string>) {
                            return lhs + rhs;
                        } else if constexpr (std::is_same_v<Left, double> && std::is_same_v<Right, double>) {
                            return lhs + rhs;
                        }

                        throw runtime_error(expr.operator_, "Operands must be two numbers or two strings.");
                    },
                    lhs,
                    rhs);
            };
        }

        else {
            static_assert(false, "Unexpected binary operator.");
        }
    }

    template <const flat_grouping_expr& expr>
    static constexpr auto generate_expr() {
        return visit<expr.expr_>();
    }

    template <const flat_literal_expr& expr>
    static constexpr auto generate_expr() {
        constexpr auto literal = static_visit_v<expr.value_>;
        static_assert(literal != none);

        value_t value(std::in_place_index<expr.value_.index() - 1>, literal);

        return [=](program_state&) -> value_t { return value; };
    }

    template <const flat_unary_expr& expr>
    static constexpr auto generate_expr() {
        auto right = visit<expr.right_>();

        if constexpr (expr.operator_.type_ == token_type::bang) {
            return [=](program_state& state) -> value_t {
                value_t value = right(state);
                value = !is_truthy(value);
                return value;
            };
        }

        else if constexpr (expr.operator_.type_ == token_type::minus) {
            return [=](program_state& state) -> value_t {
                value_t value = right(state);
                double& number = check_number_operand<expr.operator_>(value);
                number = -number;
                return value;
            };
        }

        else {
            static_assert(false, "Unexpected unary operator.");
        }
    }

    template <const flat_variable_expr& expr>
    static constexpr auto generate_expr() {
        return [](program_state& state) -> value_t { return state.env_.get(expr.name_); };
    }
};

template <const auto& ast>
constexpr auto generate_code() {
    return code_generator<ast>::generate();
}

}  // namespace ctlox::v2