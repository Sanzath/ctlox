#pragma once

#include <ctlox/v2/environment.hpp>
#include <ctlox/v2/exception.hpp>
#include <ctlox/v2/expression.hpp>
#include <ctlox/v2/flat_ast.hpp>
#include <ctlox/v2/statement.hpp>
#include <ctlox/v2/static_visit.hpp>
#include <ctlox/v2/types.hpp>

#include <functional>
#include <print>

namespace ctlox::v2 {

template <typename Fn>
concept _print_fn = std::invocable<Fn, const value_t&>;

struct default_print_fn {
    static void operator()(const value_t& value) {
        std::visit([](const auto& val) { std::print("{}\n", val); }, value);
    }
};

static_assert(_print_fn<default_print_fn>);

template <_print_fn PrintFn = default_print_fn>
struct program_state_t {
    constexpr program_state_t(program_state_t&&) noexcept = default;
    constexpr program_state_t& operator=(program_state_t&&) noexcept = default;
    constexpr program_state_t(const program_state_t&) = delete;
    constexpr program_state_t& operator=(const program_state_t&) = delete;

    constexpr explicit program_state_t(PrintFn print_fn = PrintFn {})
        : print_fn_(std::move(print_fn)) { }

    constexpr explicit program_state_t(program_state_t* enclosng)
        : env_(&enclosng->env_)
        , print_fn_(enclosng->print_fn_) { }

    constexpr void print(const value_t& value) const { print_fn_(value); }

    [[nodiscard]] constexpr program_state_t open_scope() { return program_state_t(this); }

    [[nodiscard]] constexpr value_t get_var(const token_t& name) const { return env_.get(name); }
    constexpr void define_var(const token_t& name, const value_t& value) { env_.define(name, value); }
    constexpr void assign_var(const token_t& name, const value_t& value) { env_.assign(name, value); }

private:
    environment env_;
    PrintFn print_fn_;
};

template <typename T>
concept _not_void = !std::is_void_v<T>;

template <typename T>
concept _partial_program_state = requires(T& state, const token_t& var_name, const value_t& value) {
    { state.print(value) };
    { state.open_scope() } -> _not_void;
    { state.get_var(var_name) } -> std::convertible_to<value_t>;
    { state.define_var(var_name, value) };
    { state.assign_var(var_name, value) };
};

template <typename T>
concept program_state = _partial_program_state<T> && requires(T& state) {
    { state.open_scope() } -> _partial_program_state;
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
    requires _flat_ast<decltype(ast)>
struct code_generator : _code_generator_base {
    static constexpr auto generate() {
        auto root_block = generate_block<ast.root_block_>();
        return [=]<typename PrintFn = default_print_fn>(PrintFn print_fn = default_print_fn {}) {
            program_state_t state(std::move(print_fn));

            root_block(state);
        };
    }

private:
    template <const flat_stmt_list& stmts>
    static constexpr auto generate_block() {
        using index_sequence = std::make_index_sequence<stmts.size()>;

        return []<std::size_t... I>(std::index_sequence<I...>) {
            return [... stmt = visit<stmts[I]>()](program_state auto& state) -> void { (stmt(state), ...); };
        }(index_sequence {});
    }

    template <flat_stmt_ptr ptr>
    static constexpr auto visit() {
        return generate_stmt<static_visit_v<ast[ptr]>>();
    }

    template <flat_expr_ptr ptr>
    static constexpr auto visit() {
        return generate_expr<static_visit_v<ast[ptr]>>();
    }

    template <const flat_block_stmt& stmt>
    static constexpr auto generate_stmt() {
        auto block = generate_block<stmt.statements_>();
        return [=](program_state auto& state) -> void {
            program_state auto block_state = state.open_scope();
            block(block_state);
        };
    }

    template <const flat_expression_stmt& stmt>
    static constexpr auto generate_stmt() {
        auto expr = visit<stmt.expression_>();
        return [=](program_state auto& state) -> void { expr(state); };
    }

    template <const flat_print_stmt& stmt>
    static constexpr auto generate_stmt() {
        auto expr = visit<stmt.expression_>();
        return [=](program_state auto& state) -> void { state.print(expr(state)); };
    }

    template <const flat_var_stmt& stmt>
    static constexpr auto generate_stmt() {
        if constexpr (stmt.initializer_ != flat_nullptr) {
            auto expr = visit<stmt.initializer_>();
            return [=](program_state auto& state) -> void { state.define_var(stmt.name_, expr(state)); };
        } else {
            return [=](program_state auto& state) -> void { state.define_var(stmt.name_, nil); };
        }
    }

    template <const flat_assign_expr& expr>
    static constexpr auto generate_expr() {
        auto right = visit<expr.value_>();
        return [=](program_state auto& state) -> value_t {
            auto value = right(state);
            state.assign_var(expr.name_, value);
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
            return [=](program_state auto& state) -> value_t {
                value_t lhs = left(state);
                value_t rhs = right(state);
                auto [lhs_number, rhs_number] = check_number_operands<expr.operator_>(lhs, rhs);
                return number_op(lhs_number, rhs_number);
            };
        }

        else if constexpr (value_op != none) {
            return [=](program_state auto& state) -> value_t {
                value_t lhs = left(state);
                value_t rhs = right(state);
                return value_op(lhs, rhs);
            };
        }

        else if constexpr (type == token_type::plus) {
            return [=](program_state auto& state) -> value_t {
                value_t lhs = left(state);
                value_t rhs = right(state);

                if (auto [left, right] = std::pair(std::get_if<std::string>(&lhs), std::get_if<std::string>(&rhs));
                    left && right) {
                    return *left + *right;
                }
                if (auto [left, right] = std::pair(std::get_if<double>(&lhs), std::get_if<double>(&rhs));
                    left && right) {
                    return *left + *right;
                }

                throw runtime_error(expr.operator_, "Operands must be two numbers or two strings.");
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

        return [=](program_state auto&) -> value_t {
            return value_t { std::in_place_index<expr.value_.index() - 1>, literal };
        };
    }

    template <const flat_unary_expr& expr>
    static constexpr auto generate_expr() {
        auto right = visit<expr.right_>();

        if constexpr (expr.operator_.type_ == token_type::bang) {
            return [=](program_state auto& state) -> value_t {
                value_t value = right(state);
                value = !is_truthy(value);
                return value;
            };
        }

        else if constexpr (expr.operator_.type_ == token_type::minus) {
            return [=](program_state auto& state) -> value_t {
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
        return [](program_state auto& state) -> value_t { return state.get_var(expr.name_); };
    }
};

template <const auto& ast>
constexpr auto generate_code() {
    return code_generator<ast>::generate();
}

}  // namespace ctlox::v2