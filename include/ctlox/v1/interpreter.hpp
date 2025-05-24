#pragma once

#include <ctlox/v1/ct.hpp>
#include <ctlox/v1/types.hpp>

namespace ctlox::v1 {

struct env {
    template <string _name, typename Value>
    struct entry_t {
        static constexpr inline auto name = _name;
        using value = Value;
    };

    using block_separator = entry_t<"\x1D"_ct, value_t<none>>;
    using end_of_environment = entry_t<"\x1E"_ct, value_t<none>>;

    template <typename... Entries>
    struct env_t {
        struct given_entries {
            using has_f0 = void;
            template <accepts_pack C>
            using f0 = calln<C, Entries...>;
        };
    };

    using new_env = env_t<end_of_environment>;

    // get_impl: drop until name is found
    // error if not found
    struct get_impl {
        enum class strategy {
            found,
            search,
            not_found,
        };

        static constexpr inline strategy classify(auto searched_name, auto entry_name)
        {
            if (entry_name == end_of_environment::name)
                return strategy::not_found;
            return (entry_name == searched_name) ? strategy::found : strategy::search;
        }

        template <strategy s = strategy::not_found>
        struct impl {
            template <string>
            struct error_undefined_variable_ { };
            template <typename C, typename Name, typename...>
            using f = call1<errored, error_undefined_variable_<Name::value>>;
        };

        template <>
        struct impl<strategy::found> {
            template <typename C, typename Name, typename Entry, typename... Entries>
            using f = call1<C, typename Entry::value>;
        };

        template <>
        struct impl<strategy::search> {
            template <typename C, typename Name, typename Entry, typename Next, typename... Entries>
            using f = impl<classify(Name::value, Next::name)>::template f<
                C, Name, Next, Entries...>;
        };

        using has_fn = void;
        template <accepts_one C, typename Name, typename Entry, typename... Entries>
        using fn = impl<classify(Name::value, Entry::name)>::template f<
            C, Name, Entry, Entries...>;
    };

    // declare_impl:
    // - rotate until:
    //   - block_separator: add value
    //   - name found: update value
    // - then rotate back
    struct declare_impl {
        enum class strategy {
            found,
            search,
            not_found,
        };

        static constexpr inline strategy classify(auto new_entry_name, auto entry_name)
        {
            if (entry_name == end_of_environment::name || entry_name == block_separator::name)
                return strategy::not_found;
            return (entry_name == new_entry_name) ? strategy::found : strategy::search;
        }

        template <strategy s = strategy::not_found>
        struct impl {
            // declare new entry
            template <
                typename C, std::size_t _index, typename NewEntry,
                typename Entry, typename... Entries>
            using f = calln<
                compose<rotate<(sizeof...(Entries) + 2 - _index) % (sizeof...(Entries) + 2)>, applied<env_t>, C>,
                NewEntry, Entry, Entries...>;
        };

        template <>
        struct impl<strategy::found> {
            // replace existing entry
            template <
                typename C, std::size_t _index, typename NewEntry,
                typename Entry, typename... Entries>
            using f = calln<
                compose<rotate<(sizeof...(Entries) + 1 - _index) % (sizeof...(Entries) + 1)>, applied<env_t>, C>,
                NewEntry, Entries...>;
        };

        template <>
        struct impl<strategy::search> {
            template <
                typename C, std::size_t _index, typename NewEntry,
                typename Entry, typename Next, typename... Entries>
            using f = impl<classify(NewEntry::name, Next::name)>::template f<
                C, _index + 1, NewEntry, Next, Entries..., Entry>;
        };

        using has_fn = void;
        template <accepts_one C, typename NewEntry, typename Entry, typename... Entries>
        using fn = impl<classify(NewEntry::name, Entry::name)>::template f<
            C, 0, NewEntry, Entry, Entries...>;
    };

    // assign_impl: rotate until name found, update value, then rotate back
    // error if not found
    struct assign_impl {
        enum class strategy {
            found,
            search,
            not_found,
        };

        static constexpr inline strategy classify(auto new_entry_name, auto entry_name)
        {
            if (entry_name == end_of_environment::name)
                return strategy::not_found;
            return (entry_name == new_entry_name) ? strategy::found : strategy::search;
        }

        template <strategy s = strategy::not_found>
        struct impl {
            static_assert(false, "Undefined variable");
        };

        template <>
        struct impl<strategy::found> {
            template <
                typename C, std::size_t _index, typename NewEntry,
                typename Entry, typename... Entries>
            using f = calln<
                compose<rotate<(sizeof...(Entries) + 1 - _index) % (sizeof...(Entries) + 1)>, applied<env_t>, C>,
                NewEntry, Entries...>;
        };

        template <>
        struct impl<strategy::search> {
            template <
                typename C, std::size_t _index, typename NewEntry,
                typename Entry, typename Next, typename... Entries>
            using f = impl<classify(NewEntry::name, Next::name)>::template f<
                C, _index + 1, NewEntry, Next, Entries..., Entry>;
        };

        using has_fn = void;
        template <accepts_one C, typename NewEntry, typename Entry, typename... Entries>
        using fn = impl<classify(NewEntry::name, Entry::name)>::template f<
            C, 0, NewEntry, Entry, Entries...>;
    };

    using push_scope_impl = prepend<block_separator>;

    struct pop_scope_impl {
        template <bool is_block_separator = false>
        struct impl {
            template <typename C, typename Entry, typename... Entries>
            using f = impl<std::is_same_v<Entry, block_separator>>::template f<C, Entries...>;
        };

        template <>
        struct impl<true> {
            template <typename C, typename... Entries>
            using f = calln<C, Entries...>;
        };

        using has_fn = void;
        template <accepts_pack C, typename Entry, typename... Entries>
        using fn = impl<std::is_same_v<Entry, block_separator>>::template f<C, Entries...>;
    };

    template <typename Environment, string _name>
    using get = run<
        typename Environment::given_entries,
        prepend<value_t<_name>>,
        get_impl,
        returned>;

    template <typename Environment, string _name, typename Value>
    using declare = run<
        typename Environment::given_entries,
        prepend<entry_t<_name, Value>>,
        declare_impl,
        returned>;

    template <typename Environment, string _name, typename Value>
    using assign = run<
        typename Environment::given_entries,
        prepend<entry_t<_name, Value>>,
        assign_impl,
        returned>;

    template <typename Environment>
    using push_scope = run<
        typename Environment::given_entries,
        push_scope_impl,
        applied<env_t>,
        returned>;

    template <typename Environment>
    using pop_scope = run<
        typename Environment::given_entries,
        pop_scope_impl,
        applied<env_t>,
        returned>;
};

struct interpret_ct {
private:
    struct end_of_program { };

    template <typename T>
    static constexpr bool is_truthy(T value)
    {
        if constexpr (std::same_as<T, nil_t>)
            return false;
        if constexpr (std::same_as<T, bool>)
            return value;
        return true; // Strings and numbers are always truthy
    }

    template <typename Left, typename Right>
    static constexpr bool equals(const Left& left, const Right& right)
    {
        if constexpr (std::same_as<Left, Right>) {
            return left == right;
        } else {
            return false;
        }
    }

    static constexpr double plus(const double& left, const double& right)
    {
        return left + right;
    }

    template <std::size_t _left, std::size_t _right>
    static constexpr auto plus(const string<_left>& left, const string<_right>& right)
    {
        return concat(left, right);
    }

    template <typename Left, typename Right>
    static constexpr auto plus(const Left&, const Right&)
    {
        static_assert(false, "Operands must be two numbers or two strings.");
        return nil;
    }

    struct evaluate;

    template <typename Operator>
    struct apply_unary_op {
        template <typename T>
        static constexpr auto apply(T value)
        {
            if constexpr (Operator::type == token_type::bang) {
                return !is_truthy(value);
            } else if constexpr (Operator::type == token_type::minus) {
                static_assert(std::same_as<T, double>);
                return -value;
            } else {
                static_assert(false, "logic error: unhandled case");
                return nil;
            }
        }

        using has_fn = void;
        template <typename C, typename Environment, typename Value, typename... Ts>
        using fn = calln<C, Environment, value_t<apply(Value::value)>, Ts...>;
    };

    template <typename LeftValue, typename Operator>
    struct apply_binary_op {
        template <typename T, typename U>
        static constexpr auto apply(T left, U right)
        {
            if constexpr (Operator::type == token_type::greater) {
                static_assert(std::same_as<decltype(left), double>);
                static_assert(std::same_as<decltype(right), double>);
                return left > right;
            }

            if constexpr (Operator::type == token_type::greater_equal) {
                static_assert(std::same_as<decltype(left), double>);
                static_assert(std::same_as<decltype(right), double>);
                return left >= right;
            }

            if constexpr (Operator::type == token_type::less) {
                static_assert(std::same_as<decltype(left), double>);
                static_assert(std::same_as<decltype(right), double>);
                return left < right;
            }

            if constexpr (Operator::type == token_type::less_equal) {
                static_assert(std::same_as<decltype(left), double>);
                static_assert(std::same_as<decltype(right), double>);
                return left <= right;
            }

            if constexpr (Operator::type == token_type::equal_equal) {
                return equals(left, right);
            }

            if constexpr (Operator::type == token_type::bang_equal) {
                return equals(left, right);
            }

            if constexpr (Operator::type == token_type::minus) {
                static_assert(std::same_as<decltype(left), double>);
                static_assert(std::same_as<decltype(right), double>);
                return left - right;
            }

            if constexpr (Operator::type == token_type::plus) {
                return plus(left, right);
            }

            if constexpr (Operator::type == token_type::slash) {
                static_assert(std::same_as<decltype(left), double>);
                static_assert(std::same_as<decltype(right), double>);
                return left / right;
            }

            if constexpr (Operator::type == token_type::star) {
                static_assert(std::same_as<decltype(left), double>);
                static_assert(std::same_as<decltype(right), double>);
                return left * right;
            }
        }

        using has_fn = void;
        template <typename C, typename Environment, typename RightValue, typename... Ts>
        using fn = calln<C, Environment, value_t<apply(LeftValue::value, RightValue::value)>, Ts...>;
    };

    template <typename Operator, typename RightExpr>
    struct evaluate_right {
        using has_fn = void;
        template <typename C, typename Environment, typename LeftValue, typename... Ts>
        using fn = calln<
            compose<evaluate, apply_binary_op<LeftValue, Operator>, C>,
            Environment, RightExpr, Ts...>;
    };

    template <typename Name>
    struct apply_assign_op {
        using has_fn = void;
        template <typename C, typename Environment, typename Value, typename... Ts>
        using fn = calln<C, env::assign<Environment, Name::lexeme, Value>, Value, Ts...>;
    };

    struct evaluate {
        template <typename Expr>
        struct impl {
            static_assert(false, "logic error: unhandled case");
        };

        template <auto _value>
        struct impl<literal_expr<_value>> {
            template <typename C, typename Environment, typename... Ts>
            using f = calln<C, Environment, value_t<_value>, Ts...>;
        };

        template <typename Name>
        struct impl<variable_expr<Name>> {
            template <typename C, typename Environment, typename... Ts>
            using f = calln<C, Environment, env::get<Environment, Name::lexeme>, Ts...>;
        };

        template <typename Name, typename ValueExpr>
        struct impl<assign_expr<Name, ValueExpr>> {
            template <typename C, typename Environment, typename... Ts>
            using f = calln<
                compose<evaluate, apply_assign_op<Name>, C>,
                Environment, ValueExpr, Ts...>;
        };

        template <typename Expr>
        struct impl<grouping_expr<Expr>> {
            template <typename C, typename Environment, typename... Ts>
            using f = calln<compose<evaluate, C>, Environment, Expr, Ts...>;
        };

        template <typename Operator, typename Expr>
        struct impl<unary_expr<Operator, Expr>> {
            template <typename C, typename Environment, typename... Ts>
            using f = calln<
                compose<evaluate, apply_unary_op<Operator>, C>,
                Environment, Expr, Ts...>;
        };

        template <typename LeftExpr, typename Operator, typename RightExpr>
        struct impl<binary_expr<LeftExpr, Operator, RightExpr>> {
            template <typename C, typename Environment, typename... Ts>
            using f = calln<
                compose<evaluate, evaluate_right<Operator, RightExpr>, C>,
                Environment, LeftExpr, Ts...>;
        };

        using has_fn = void;
        template <typename C, typename Environment, typename Expr, typename... Ts>
        using fn = impl<Expr>::template f<C, Environment, Ts...>;
    };

    template <string name>
    struct bind_variable {
        using has_fn = void;
        template <typename C, typename Environment, typename Value, typename... Ts>
        using fn = calln<C, env::declare<Environment, name, Value>, Ts...>;
    };

    struct execute;

    template <std::size_t block_size>
    struct execute_block {
        using has_fn = void;
        template <accepts_pack C, typename Environment, typename... Ts>
        using fn = calln<compose<execute, execute_block<block_size - 1>, C>, Environment, Ts...>;
    };

    template <>
    struct execute_block<0> {
        using has_fn = void;
        template <accepts_pack C, typename Environment, typename... Ts>
        using fn = calln<C, env::pop_scope<Environment>, Ts...>;
    };

    struct execute {
        using ignore_result = drop_at<1>;
        using append_result = move_to_back<1>;

        template <typename Stmt>
        struct impl {
            static_assert(false, "logic error: unhandled case");
        };

        template <typename Expr>
        struct impl<expression_stmt<Expr>> {
            template <typename C, typename Environment, typename... Ts>
            using f = calln<compose<evaluate, ignore_result, C>, Environment, Expr, Ts...>;
        };

        template <typename Expr>
        struct impl<print_stmt<Expr>> {
            template <typename C, typename Environment, typename... Ts>
            using f = calln<compose<evaluate, append_result, C>, Environment, Expr, Ts...>;
        };

        template <typename Name, typename Initialiser>
        struct impl<var_stmt<Name, Initialiser>> {
            template <typename C, typename Environment, typename... Ts>
            using f = calln<compose<evaluate, bind_variable<Name::lexeme>, C>, Environment, Initialiser, Ts...>;
        };

        template <typename... Statements>
        struct impl<block_stmt<Statements...>> {
            template <typename C, typename Environment, typename... Ts>
            using f = calln<
                compose<execute_block<sizeof...(Statements)>, C>,
                env::push_scope<Environment>, Statements..., Ts...>;
        };

        using has_fn = void;
        template <typename C, typename Environment, typename Stmt, typename... Ts>
        using fn = impl<Stmt>::template f<C, Environment, Ts...>;
    };

    struct interpret {
        template <bool is_eop>
        struct impl {
            template <typename C, typename Environment, typename... Ts>
            using f = calln<compose<execute, interpret, C>, Environment, Ts...>;
        };

        template <>
        struct impl<true> {
            // found sentinel: done interpreting
            template <typename C, typename Environment, typename EndOfProgram, typename... Output>
            using f = calln<C, Output...>;
        };

        using has_fn = void;
        template <accepts_pack C, typename Environment, typename T0, typename... Ts>
        using fn = impl<std::is_same_v<T0, end_of_program>>::template f<C, Environment, T0, Ts...>;
    };

public:
    using has_fn = void;
    template <accepts_pack C, typename... Statements>
    using fn = calln<
        compose<interpret, C>,
        env::new_env, Statements..., end_of_program>;
};
}
