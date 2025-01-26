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
        //static constexpr inline call_arity accepts = F::accepts;
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

    // example zero-to-one: given
    template <typename T>
    struct given {
        static constexpr inline auto accepts = call_arity::zero;

        template <accepts_one C>
        using f = call<C, T>;
    };

    // example zero-to-pack: given_some
    template <typename... Ts>
    struct given_some {
        static constexpr inline auto accepts = call_arity::zero;

        template <accepts_pack C>
        using f = call<C, Ts...>;
    };

    // for debugging: produce a compiler error with to_error[_p]'s input
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

        using has_f1 = void;
        using has_fn = void;

        template <typename C, typename T>
        using f1 = typename print<T>::type;
        template <typename C, typename... Ts>
        using fn = typename print<Ts...>::type;
    };

    namespace take2 {
        template <typename C>
        struct cont_traits {
            static constexpr inline bool has_f0 = requires { typename C::has_f0; };
            static constexpr inline bool has_f1 = requires { typename C::has_f1; };
            static constexpr inline bool has_fn = requires { typename C::has_fn; };
        };

        template <typename F, typename... Fs>
        struct cont_traits<composition<F, Fs...>> : cont_traits<F> {};

        template <typename C>
        concept accepts_zero = cont_traits<C>::has_f0;

        template <typename C>
        concept accepts_one = cont_traits<C>::has_f1;

        template <typename C>
        concept accepts_pack = cont_traits<C>::has_fn;

        struct deferred;

        template <accepts_zero C>
        struct call0_impl {
            using f0 = C::template f0<deferred>;
        };

        template <accepts_one C>
        struct call1_impl {
            template <typename T>
            using f1 = C::template f1<deferred, T>;
        };

        template <accepts_pack C>
        struct calln_impl {
            template <typename... Ts>
            using fn = C::template fn<deferred, Ts...>;
        };

        template <accepts_zero C>
        using call0 = typename call0_impl<C>::f0;
        template <accepts_one C, typename T>
        using call1 = typename call1_impl<C>::template f1<T>;
        template <accepts_pack C, typename... Ts>
        using calln = typename calln_impl<C>::template fn<Ts...>;

        // default continuation: deferral
        struct deferred {
            using has_f0 = void;
            using has_f1 = void;
            using has_fn = void;

            struct capture_0 {
                using has_f0 = void;
                template <accepts_zero C>
                using f0 = call0<C>;
            };

            template <typename T>
            struct capture_1 {
                using has_f0 = void;
                template <accepts_one C>
                using f0 = call1<C, T>;
            };

            template <typename... Ts>
            struct capture_n {
                using has_f0 = void;
                template <accepts_pack C>
                using f0 = calln<C, Ts...>;
            };

            template <typename C>
            using f0 = capture_0;
            template <typename C, typename T>
            using f1 = capture_1<T>;
            template <typename C, typename... Ts>
            using fn = capture_n<Ts...>;
        };

        namespace callztests {
            using t0 = call0<deferred>;
            using t1 = call1<deferred, int>;
            using tn0 = calln<deferred>;
            using tn1 = calln<deferred, int>;
            using tn2 = calln<deferred, char, short>;
        }

        // composition
        template <accepts_zero F, typename... Fs>
        struct call0_impl<composition<F, Fs...>> {
            using f0 = F::template f0<compose<Fs...>>;
        };
        template <accepts_one F, typename... Fs>
        struct call1_impl<composition<F, Fs...>> {
            template <typename T>
            using f1 = F::template f1<compose<Fs...>, T>;
        };
        template <accepts_pack F, typename... Fs>
        struct calln_impl<composition<F, Fs...>> {
            template <typename... Ts>
            using fn = F::template fn<compose<Fs...>, Ts...>;
        };

        namespace comptests {
            struct zero_to_zero {
                using has_f0 = void;
                template <typename C>
                using f0 = call0<C>;
            };
            struct zero_to_one {
                using has_f0 = void;
                template <typename C>
                using f0 = call1<C, int>;
            };
            struct zero_to_pack {
                using has_f0 = void;
                template <typename C>
                using f0 = calln<C, int, char>;
            };
            struct one_to_zero {
                using has_f1 = void;
                template <typename C, typename T>
                using f1 = call0<C>;
            };
            struct one_to_one {
                using has_f1 = void;
                template <typename C, typename T>
                using f1 = call1<C, T>;
            };
            struct one_to_pack {
                using has_f1 = void;
                template <typename C, typename T>
                using f1 = calln<C, T, T>;
            };
            struct pack_to_zero {
                using has_fn = void;
                template <typename C, typename... Ts>
                using fn = call0<C>;
            };
            struct pack_to_one {
                using has_fn = void;
                template <typename C, typename... Ts>
                using fn = call1<C, int>;
            };
            struct pack_to_pack {
                using has_fn = void;
                template <typename C, typename... Ts>
                using fn = calln<C, Ts...>;
            };

            using t1 = call0<compose<zero_to_one, one_to_pack, pack_to_zero, zero_to_zero, zero_to_pack, pack_to_pack, pack_to_one, one_to_one, one_to_one>>;
            static_assert(std::same_as<t1, deferred::capture_1<int>>);
            using t2 = call0<compose<t1, one_to_pack>>;

            using t3 = calln<compose<pack_to_one>, int, char*>;
        }

        // inferred call
        template <typename C, std::size_t N>
        struct call_impl;

        template <typename C>
            requires(accepts_zero<C>&& accepts_pack<C>)
        struct call_impl<C, 0>
        {
            static_assert(false, "Call with 0 arguments is ambiguous when C could accept either zero arguments or a pack. Use call0 or calln instead.");
        };

        template <accepts_zero C>
        struct call_impl<C, 0> {
            template <typename... Ts>
            using f = call0<C>;
        };

        template <accepts_pack C>
        struct call_impl<C, 0> {
            template <typename... Ts>
            using f = calln<C>;
        };

        template <typename C>
            requires(accepts_one<C> && accepts_pack<C>)
        struct call_impl<C, 1>
        {
            static_assert(false, "Call with 1 argument is ambiguous when C could accept either one argument or a pack. Use call1 or calln instead.");
        };

        template <accepts_one C>
        struct call_impl<C, 1> {
            template <typename T>
            using f = call1<C, T>;
        };

        template <accepts_pack C>
        struct call_impl<C, 1> {
            template <typename T>
            using f = calln<C, T>;
        };

        template <accepts_pack C, std::size_t N>
            requires(N > 1)
        struct call_impl<C, N> {
            template <typename... Ts>
            using f = calln<C, Ts...>;
        };

        // Utility to initiate a call. For use in user code only, as one of the call* functions should generally
        // be preferred to avoid ambiguities.
        template <typename C, typename... Ts>
        using call = call_impl<C, sizeof...(Ts)>::template f<Ts...>;

        namespace calltests {
            using namespace comptests;

            using t0 = call<deferred, int, char>;
            using t1 = call<zero_to_one>;
            using t2 = call<one_to_pack, int>;
            using t3 = call<pack_to_one>;
            using t4 = call<pack_to_one, int>;
            using t5 = call<pack_to_pack, int, char>;
            //using t6 = call<zero_to_one, char>;
            //using t7 = call<zero_to_pack, int, char>;
            //using t8 = call<one_to_one>;
            //using t9 = call<one_to_one, char, int>;
        }
    }

    //struct call1_impl {};
    //template 


    //namespace tests {

    //    // example one-to-pack: repeat_5 (could be repeat<N>)
    //    struct repeat_5 {
    //        static constexpr inline auto accepts = call_arity::one;

    //        template <accepts_pack C, typename Arg>
    //        using f = call<C, Arg, Arg, Arg, Arg, Arg>;
    //    };

    //    static_assert(std::is_same_v<at<3>, compose<at<3>>>);

    //    static_assert(std::is_same_v<call<identity, double>, double>);

    //    using my_composition = compose<
    //        given_some<int, double, std::string, std::size_t>,
    //        compose<at<2>, identity>,
    //        compose<identity, repeat_5>,
    //        to_list,
    //        identity
    //        //to_error
    //    >;

    //    using my_list = call<my_composition>;

    //    template <typename T>
    //    concept accepts_zero = requires{
    //        typename T::has_f0;
    //    };

    //    template <typename T>
    //    concept accepts_one = requires{
    //        typename T::has_f1;
    //    };

    //    static_assert(accepts_one<to_error>);


    //    static_assert(std::is_same_v<my_list, list<std::string, std::string, std::string, std::string, std::string>>);
    //}
}

