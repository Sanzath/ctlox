#pragma once

#include <tuple>

namespace ctlox {
    // dcall: template magic that allows alias pack size "mismatches"
    template <bool b, typename C>
    struct dependant_impl;
    template <typename C>
    struct dependant_impl<true, C> {
        using type = C;
    };

    template <typename C, std::size_t N>
    using dcall = typename dependant_impl<(N < 100'000), C>::type;

    enum class call_arity {
        zero,
        one,
        pack,
    };

    template <typename C>
    concept accepts_zero = (C::accepts == call_arity::zero);
    template <typename C>
    concept accepts_one = (C::accepts == call_arity::one);
    template <typename C>
    concept accepts_pack = (C::accepts == call_arity::pack);

    // calling continuations
    struct sentinel {
        static constexpr inline auto accepts = call_arity::one;
    };

    template <typename C>
    struct call_impl;

    template <accepts_zero C>
    struct call_impl<C> {
        template <typename... Ts>
            requires(sizeof...(Ts) == 0)
        using f = C::template f<sentinel>;
    };

    template <accepts_one C>
    struct call_impl<C> {
        template <typename T>
        using f = C::template f<sentinel, T>;
    };

    template <accepts_pack C>
    struct call_impl<C> {
        template <typename... Ts>
        using f = C::template f<sentinel, Ts...>;
    };

    template <>
    struct call_impl<sentinel> {
        template <typename T>
        using f = T;
    };

    template <typename C, typename... Ts>
    using call = call_impl<C>::template f<Ts...>;

    // composing continuations
    template <typename... Fs>
    struct composition;

    template <typename F, typename... Fs>
    struct composition<F, Fs...> {
        static constexpr inline call_arity accepts = F::accepts;
    };

    // building compositions: they must be flat
    struct compose_impl {
    private:
        template <bool _singular>
        struct recompose {
            template <typename... Fs>
            using f = composition<Fs...>;
        };

        template <>
        struct recompose<true> {
            template <typename F>
            using f = F;
        };

        template <typename... Fs>
        struct decompose;

        template <typename... CFs, typename... Fs>
        struct decompose<composition<CFs...>, Fs...> {
            template <typename... OFs>
            using f = decompose<CFs..., Fs...>::template f<OFs...>;
        };

        template <typename F, typename... Fs>
        struct decompose<F, Fs...> {
            template <typename... OFs>
            using f = decompose<Fs...>::template f<OFs..., F>;
        };
        
        template <>
        struct decompose<> {
            template <typename... OFs>
            using f = recompose<sizeof...(OFs) == 1>::template f<OFs...>;
        };

    public:
        template <typename... Fs>
        using f = decompose<Fs...>::template f<>;
    };

    template <typename... Fs>
    using compose = compose_impl::template f<Fs...>;

    // call machinery
    template <accepts_zero F, typename... Fs>
    struct call_impl<composition<F, Fs...>> {
        template <typename... Args>
            requires(sizeof...(Args) == 0)
        using f = F::template f<compose<Fs...>>;
    };

    template <accepts_one F, typename... Fs>
    struct call_impl<composition<F, Fs...>> {
        template <typename Arg>
        using f = F::template f<compose<Fs...>, Arg>;
    };

    template <accepts_pack F, typename... Fs>
    struct call_impl<composition<F, Fs...>> {
        template <typename... Args>
        using f = F::template f<compose<Fs...>, Args...>;
    };

    // identity
    struct identity {
        static constexpr inline auto accepts = call_arity::one;

        template <accepts_one C, typename Arg>
        using f = call<C, Arg>;
    };

    // list stuff
    template <typename... Ts>
    struct list {
        static constexpr std::size_t size = sizeof...(Ts);
    };

    struct to_list {
        static constexpr inline auto accepts = call_arity::pack;

        template <accepts_one C, typename... Ts>
        using f = call<C, list<Ts...>>;
    };

    struct from_list {
    private:
        template <typename T>
        struct from_list_impl;

        template <typename... Ts>
        struct from_list_impl<list<Ts...>> {
            template <typename C>
            using f = call<C, Ts...>;
        };

    public:
        static constexpr inline auto accepts = call_arity::one;

        template <accepts_pack C, typename List>
        using f = from_list_impl<List>::template f<C>;
    };

    // example pack-to-one: at
    template <std::size_t I>
    struct at {
    private:
        template <std::size_t _current>
        struct at_impl;

        template <std::size_t _current>
            requires(_current < I)
        struct at_impl<_current> {
            template <typename C, typename Dropped, typename... Ts>
            using f = dcall<at_impl<_current + 1>, sizeof...(Ts)>::template f<C, Ts...>;
        };

        template <std::size_t _current>
            requires(_current == I)
        struct at_impl<_current> {
            template <typename C, typename Found, typename... Ts>
            using f = call<C, Found>;
        };

    public:
        static constexpr inline auto accepts = call_arity::pack;

        template <accepts_one C, typename... Ts>
            requires (I < sizeof...(Ts))
        using f = dcall<at_impl<0>, sizeof...(Ts)>::template f<C, Ts...>;
    };

    // example zero-to-pack: given_some
    template <typename... Ts>
    struct given_some {
        static constexpr inline auto accepts = call_arity::zero;

        template <accepts_pack C>
        using f = call<C, Ts...>;
    };

    //
    struct to_error_p {
        static constexpr inline auto accepts = call_arity::pack;
        template <typename... Ts> struct print;
        template <typename C, typename... Ts>
        using f = typename print<Ts...>::type;
    };
    struct to_error {
        static constexpr inline auto accepts = call_arity::one;
        template <typename... Ts> struct print;
        template <typename C, typename T>
        using f = typename print<T>::type;
    };

    namespace tests {

        // example one-to-pack: repeat_5 (could be repeat<N>)
        struct repeat_5 {
            static constexpr inline auto accepts = call_arity::one;

            template <accepts_pack C, typename Arg>
            using f = call<C, Arg, Arg, Arg, Arg, Arg>;
        };

        static_assert(std::is_same_v<at<3>, compose<at<3>>>);

        static_assert(std::is_same_v<call<identity, double>, double>);

        using my_composition = compose<
            given_some<int, double, std::string, std::size_t>,
            compose<at<2>, identity>,
            compose<identity, repeat_5>,
            to_list,
            identity,
            to_error
        >;

        using my_list = call<my_composition>;



        static_assert(std::is_same_v<my_list, list<std::string, std::string, std::string, std::string, std::string>>);
    }
}

