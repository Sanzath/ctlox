#pragma once

#include <chrono>
#include <ctlox/v2/environment.hpp>
#include <ctlox/v2/exception.hpp>
#include <ctlox/v2/expression.hpp>
#include <ctlox/v2/flat_ast.hpp>
#include <ctlox/v2/literal.hpp>
#include <ctlox/v2/lox_function.hpp>
#include <ctlox/v2/program_state.hpp>
#include <ctlox/v2/resolver.hpp>
#include <ctlox/v2/statement.hpp>
#include <ctlox/v2/static_visit.hpp>

#include <functional>
#include <print>

namespace ctlox::v2 {

struct default_println_fn {
    static constexpr void operator()(const value_t& value) {
        if !consteval {
            value.visit([](const auto& val) { std::print("{}\n", val); });
        }
    }
};

struct default_clock_fn {
    static constexpr value_t operator()() {
        if !consteval {
            using double_time_point_t
                = std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<double>>;
            return double_time_point_t(std::chrono::system_clock::now()).time_since_epoch().count();
        } else {
            return 0.0;
        }
    }
};

struct default_setup_fn {
    static constexpr void operator()(environment*) { }
};

template <typename Fn>
concept _setup_fn = std::invocable<Fn, environment*>;

struct _code_generator_base {
    static constexpr bool is_truthy(const value_t& value) {
        if (value.holds<nil_t>()) {
            return false;
        }
        if (const bool* b = value.get_if<bool>()) {
            return *b;
        }
        return true;
    }

    template <const token_t& oper>
    static constexpr double& check_number_operand(value_t& value) {
        if (double* number = value.get_if<double>()) {
            return *number;
        }

        throw runtime_error(oper, "Operand must be a number.");
    }

    template <const token_t& oper>
    static constexpr std::pair<double&, double&> check_number_operands(value_t& lhs, value_t& rhs) {
        double* lhs_number = lhs.get_if<double>();
        double* rhs_number = rhs.get_if<double>();

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

    template <token_type type>
    static constexpr auto short_circuit_op_for() {
        if constexpr (type == token_type::_or)
            return [](const value_t& value) { return is_truthy(value); };
        else if constexpr (type == token_type::_and)
            return [](const value_t& value) { return !is_truthy(value); };
        else
            return none;
    }

    template <const literal_t& literal>
    static constexpr value_t materialize() {
        return value_t(std::in_place_index<literal.index() - 1>, static_visit_v<literal>);
    }
};

template <const auto& ast, const auto& bindings>
    requires _flat_ast<decltype(ast)> && _bindings<decltype(bindings)>
struct code_generator : _code_generator_base {
    static constexpr auto generate() {
        using root_block = visit_t<ast.root_block_>;
        return []<_setup_fn SetupFn = default_setup_fn>(SetupFn&& setup_fn = {}) {
            heap_t heap;
            environment globals;

            globals.define_native<1>("println", default_println_fn {});
            globals.define_native<0>("clock", default_clock_fn {});

            std::invoke(std::forward<SetupFn>(setup_fn), &globals);

            program_state_t state(&heap, &globals);

            root_block {}(state);
        };
    }

private:
    template <flat_stmt_list stmts>
    static constexpr auto visit() {
        // Allow for larger blocks without increasing -fbracket-depth
        // by (potentially recursively) chunking the block into two smaller blocks
        if constexpr (stmts.size() > 256) {
            constexpr flat_stmt_ptr middle = stmts[stmts.size() / 2];

            constexpr flat_stmt_list left { .first_ = stmts.first_, .last_ = middle };
            constexpr flat_stmt_list right { .first_ = middle, .last_ = stmts.last_ };

            using left_block = decltype(visit<left>());
            using right_block = decltype(visit<right>());
            return [](const program_state_t& state) static -> bool {
                return left_block {}(state) && right_block {}(state);
            };
        }

        else {
            using index_sequence = std::make_index_sequence<stmts.size()>;

            return []<std::size_t... I>(std::index_sequence<I...>) {
                return [](const program_state_t& state) static -> bool {
                    // && guarantees left-to-right evaluation with short-circuiting.
                    return (visit_t<stmts[I]> {}(state) && ...);
                };
            }(index_sequence {});
        }
    }

    template <flat_expr_list exprs>
    static constexpr auto visit() {
        using index_sequence = std::make_index_sequence<exprs.size()>;

        return []<std::size_t... I>(std::index_sequence<I...>) {
            return [](const program_state_t& state) static -> std::array<value_t, exprs.size()> {
                // braced list-initialization guarantees each call is sequenced before the following calls.
                return { visit_t<exprs[I]> {}(state)... };
            };
        }(index_sequence {});
    }

    template <flat_stmt_ptr ptr>
    static constexpr auto visit() {
        return generate_stmt<ptr, static_visit_v<ast[ptr]>>();
    }

    template <flat_expr_ptr ptr>
    static constexpr auto visit() {
        return generate_expr<ptr, static_visit_v<ast[ptr]>>();
    }

    template <auto v>
    using visit_t = decltype(visit<v>());

    template <flat_stmt_ptr ptr, const flat_block_stmt& stmt>
    static constexpr auto generate_stmt() {
        using block = visit_t<stmt.statements_>;
        return [](const program_state_t& state) static -> bool {
            environment env(state.env_, state.heap_, bindings.find_scope_upvalues(ptr));
            const program_state_t block_state = state.with(&env);
            return block {}(block_state);
        };
    }

    template <flat_stmt_ptr, const flat_break_stmt& stmt>
    static constexpr auto generate_stmt() {
        return [](const program_state_t& state) static -> bool {
            (*state.break_slot_)();
            return false;
        };
    }

    template <flat_stmt_ptr, const flat_expression_stmt& stmt>
    static constexpr auto generate_stmt() {
        using expr = visit_t<stmt.expression_>;
        return [](const program_state_t& state) static -> bool {
            expr {}(state);
            return true;
        };
    }

    template <flat_stmt_ptr ptr, const flat_function_stmt& stmt>
    static constexpr auto generate_stmt() {
        using body = visit_t<stmt.body_>;
        constexpr std::span<const token_t> params = ast.range(stmt.params_);
        constexpr std::span<const var_index_t> closure_upvales = bindings.find_closure_upvalues(ptr);
        constexpr std::span<const var_index_t> scope_upvalues = bindings.find_scope_upvalues(ptr);

        using function_def = lox_function<stmt.name_.lexeme_, params, closure_upvales, scope_upvalues, body>;

        return [](const program_state_t& state) static -> bool {
            // Workaround: predefine the name in case the function captures itself.
            state.env_->define(stmt.name_.lexeme_, nil);
            state.env_->assign(stmt.name_, function(function_def(state.env_)));
            return true;
        };
    }

    template <flat_stmt_ptr, const flat_if_stmt& stmt>
    static constexpr auto generate_stmt() {
        using condition = visit_t<stmt.condition_>;
        using then_branch = visit_t<stmt.then_branch_>;

        if constexpr (stmt.else_branch_ != flat_nullptr) {
            using else_branch = visit_t<stmt.else_branch_>;

            return [](const program_state_t& state) static -> bool {
                if (is_truthy(condition {}(state))) {
                    return then_branch {}(state);
                } else {
                    return else_branch {}(state);
                }
            };
        }

        else {
            return [](const program_state_t& state) static -> bool {
                if (is_truthy(condition {}(state))) {
                    return then_branch {}(state);
                } else {
                    return true;
                }
            };
        }
    }

    template <flat_stmt_ptr, const flat_return_stmt& stmt>
    static constexpr auto generate_stmt() {
        if constexpr (stmt.value_ != flat_nullptr) {
            using value = visit_t<stmt.value_>;
            return [](const program_state_t& state) static -> bool {
                (*state.return_slot_)(value {}(state));
                return false;
            };
        }

        else {
            return [](const program_state_t& state) static -> bool {
                (*state.return_slot_)(nil);
                return false;
            };
        }
    }

    template <flat_stmt_ptr, const flat_var_stmt& stmt>
    static constexpr auto generate_stmt() {
        if constexpr (stmt.initializer_ != flat_nullptr) {
            using expr = visit_t<stmt.initializer_>;
            return [](const program_state_t& state) static -> bool {
                state.env_->define(stmt.name_.lexeme_, expr {}(state));
                return true;
            };
        }

        else {
            return [](const program_state_t& state) static -> bool {
                state.env_->define(stmt.name_.lexeme_, nil);
                return true;
            };
        }
    }

    template <flat_stmt_ptr, const flat_while_stmt& stmt>
    static constexpr auto generate_stmt() {
        using condition = visit_t<stmt.condition_>;
        using body = visit_t<stmt.body_>;

        return [](const program_state_t& state) static -> bool {
            break_slot break_slot;
            const program_state_t loop_state = state.with(&break_slot);

            bool continue_execution = true;
            while (continue_execution && is_truthy(condition {}(loop_state))) {
                continue_execution = body {}(loop_state);
            }

            if (break_slot) {
                continue_execution = true;
            }
            // if continue_execution is false for another reason, propagate

            return continue_execution;
        };
    }

    template <flat_expr_ptr ptr, const flat_assign_expr& expr>
    static constexpr auto generate_expr() {
        using right = visit_t<expr.value_>;
        constexpr var_index_t local = bindings.find_local(ptr);

        if constexpr (local) {
            return [](const program_state_t& state) static -> value_t {
                value_t value = right {}(state);
                state.env_->assign_at(local.env_depth_, local.env_index_, value);
                return value;
            };
        }

        else {
            return [](const program_state_t& state) static -> value_t {
                value_t value = right {}(state);
                state.globals_->assign(expr.name_, value);
                return value;
            };
        }
    }

    template <flat_expr_ptr, const flat_binary_expr& expr>
    static constexpr auto generate_expr() {
        using left = visit_t<expr.left_>;
        using right = visit_t<expr.right_>;

        constexpr token_type type = expr.operator_.type_;
        using number_op = decltype(number_op_for<type>());
        using value_op = decltype(value_op_for<type>());

        if constexpr (number_op {} != none) {
            return [](const program_state_t& state) static -> value_t {
                value_t lhs = left {}(state);
                value_t rhs = right {}(state);
                auto [lhs_number, rhs_number] = check_number_operands<expr.operator_>(lhs, rhs);
                return number_op {}(lhs_number, rhs_number);
            };
        }

        else if constexpr (value_op {} != none) {
            return [](const program_state_t& state) static -> value_t {
                value_t lhs = left {}(state);
                value_t rhs = right {}(state);
                return value_op {}(lhs, rhs);
            };
        }

        else if constexpr (type == token_type::plus) {
            return [](const program_state_t& state) static -> value_t {
                value_t lhs = left {}(state);
                value_t rhs = right {}(state);

                if (auto [left, right] = std::pair(lhs.get_if<std::string>(), rhs.get_if<std::string>());
                    left && right) {
                    return *left + *right;
                }
                if (auto [left, right] = std::pair(lhs.get_if<double>(), rhs.get_if<double>()); left && right) {
                    return *left + *right;
                }

                throw runtime_error(expr.operator_, "Operands must be two numbers or two strings.");
            };
        }

        else {
            static_assert(false, "Unexpected binary operator.");
        }
    }

    template <flat_expr_ptr, const flat_call_expr& expr>
    static constexpr auto generate_expr() {
        using callee_fn = visit_t<expr.callee_>;
        using arguments_fn = visit_t<expr.arguments_>;

        return [](const program_state_t& state) static -> value_t {
            value_t callee = callee_fn {}(state);
            const auto arguments = arguments_fn {}(state);

            function* fn = callee.get_if<function>();

            if (!fn) {
                throw runtime_error(expr.paren_, "Can only call functions and classes.");
            }

            if (arguments.size() != fn->arity()) {
                throw runtime_error(expr.paren_, "Incorrect argument count.");
            }

            return (*fn)(state, std::span(arguments));
        };
    }

    template <flat_expr_ptr, const flat_grouping_expr& expr>
    static constexpr auto generate_expr() {
        return visit<expr.expr_>();
    }

    template <flat_expr_ptr, const flat_literal_expr& expr>
    static constexpr auto generate_expr() {
        static_assert(!std::holds_alternative<none_t>(expr.value_));

        return [](const program_state_t&) static -> value_t { return materialize<expr.value_>(); };
    }

    template <flat_expr_ptr, const flat_logical_expr& expr>
    static constexpr auto generate_expr() {
        using left = visit_t<expr.left_>;
        using right = visit_t<expr.right_>;

        using short_cirtuit_op = decltype(short_circuit_op_for<expr.operator_.type_>());
        static_assert(short_cirtuit_op {} != none, "Unexpected logical operator.");

        return [](const program_state_t& state) static -> value_t {
            value_t lhs = left {}(state);

            if (short_cirtuit_op {}(lhs))
                return lhs;

            return right {}(state);
        };
    }

    template <flat_expr_ptr, const flat_unary_expr& expr>
    static constexpr auto generate_expr() {
        using right = visit_t<expr.right_>;

        if constexpr (expr.operator_.type_ == token_type::bang) {
            return [](const program_state_t& state) static -> value_t {
                value_t value = right {}(state);
                value = !is_truthy(value);
                return value;
            };
        }

        else if constexpr (expr.operator_.type_ == token_type::minus) {
            return [](const program_state_t& state) static -> value_t {
                value_t value = right {}(state);
                double& number = check_number_operand<expr.operator_>(value);
                number = -number;
                return value;
            };
        }

        else {
            static_assert(false, "Unexpected unary operator.");
        }
    }

    template <flat_expr_ptr ptr, const flat_variable_expr& expr>
    static constexpr auto generate_expr() {
        return lookup_variable<ptr, expr.name_>();
    }

    template <flat_expr_ptr ptr, const token_t& name>
    static constexpr auto lookup_variable() {
        constexpr var_index_t local = bindings.find_local(ptr);

        if constexpr (local) {
            return [](const program_state_t& state) static -> value_t {
                return state.env_->get_at(local.env_depth_, local.env_index_);
            };
        }

        else {
            return [](const program_state_t& state) static -> value_t { return state.globals_->get(name); };
        }
    }
};

template <const auto& ast, const auto& locals>
constexpr auto generate_code() {
    return code_generator<ast, locals>::generate();
}

}  // namespace ctlox::v2