#pragma once

#include <concepts>
#include <type_traits>

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

// continuation traits
template <typename C>
struct cont_traits {
    static constexpr inline bool has_f0 = requires { typename C::has_f0; };

    static constexpr inline bool has_f1 = requires { typename C::has_f1; };
    static constexpr inline bool has_fn = requires { typename C::has_fn; };

    static constexpr inline bool has_z1 = requires { typename C::has_z1; };
    static constexpr inline bool has_zn = requires { typename C::has_zn; };
};

// A continuation which can accept exactly 0 arguments through its f0 alias.
// Usually used to initiate a composition.
template <typename C>
concept initiator = cont_traits<C>::has_f0;

// A continuation which accepts another continuation and exactly one argument through its f1 alias.
template <typename C>
concept continues_one = cont_traits<C>::has_f1;

// A continuation which accepts exactly one argument through its z1 alias, terminating a chain.
template <typename C>
concept terminates_one = cont_traits<C>::has_z1;

// A continuation which is callable with call1.
template <typename C>
concept accepts_one = continues_one<C> || terminates_one<C>;

// A continuation which accepts another continuation and a pack of arguments through its fn alias.
template <typename C>
concept continues_pack = cont_traits<C>::has_fn;

// A continuation which accepts a pack of arguments through its zn alias, terminating a chain.
template <typename C>
concept terminates_pack = cont_traits<C>::has_zn;

// A continuation which is callable with calln.
template <typename C>
concept accepts_pack = continues_pack<C> || terminates_pack<C>;

struct deferred;

// calling continuations
template <initiator C>
struct initiate_impl {
    using f0 = C::template f0<deferred>;
};

template <typename C>
struct call1_impl {
    static_assert(false, "C must model accepts_one");
};

template <continues_one C>
struct call1_impl<C> {
    template <typename T>
    using f1 = C::template f1<deferred, T>;
};

template <terminates_one C>
struct call1_impl<C> {
    template <typename T>
    using f1 = C::template z1<T>;
};

template <typename C>
struct calln_impl {
    static_assert(false, "C must model accepts_pack");
};

template <continues_pack C>
struct calln_impl<C> {
    template <typename... Ts>
    using fn = C::template fn<deferred, Ts...>;
};

template <terminates_pack C>
struct calln_impl<C> {
    template <typename... Ts>
    using fn = C::template zn<Ts...>;
};

// calls the next continuation of a specified "arity"
template <initiator C>
using initiate = typename initiate_impl<C>::f0;
template <accepts_one C, typename T>
using call1 = typename call1_impl<C>::template f1<T>;
template <accepts_pack C, typename... Ts>
using calln = typename calln_impl<C>::template fn<Ts...>;

// inferred call
template <typename C, std::size_t N>
struct call_impl {
    static_assert(false, "Invalid continuation for the given number of arguments.");
};

template <typename C>
    requires(initiator<C> && accepts_pack<C>)
struct call_impl<C, 0> {
    static_assert(false, "Call with 0 arguments is ambiguous when C could accept either zero arguments or a pack. Use initiate or calln instead.");
};

template <initiator C>
struct call_impl<C, 0> {
    template <typename... Ts>
    using f = initiate<C>;
};

template <accepts_pack C>
struct call_impl<C, 0> {
    template <typename... Ts>
    using f = calln<C>;
};

template <typename C>
    requires(accepts_one<C> && accepts_pack<C>)
struct call_impl<C, 1> {
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

// given_one: initiator which passes its template argument to the continuation.
template <typename T>
struct given_one {
    using has_f0 = void;

    template <accepts_one C>
    using f0 = call1<C, T>;
};

// given_pack: initiator which passes its template arguments to the continuation.
template <typename... Ts>
struct given_pack {
    using has_f0 = void;

    template <accepts_pack C>
    using f0 = calln<C, Ts...>;
};

// deferred: terminal which returns given_one or given_pack,
// which can be used to initiate another composition.
struct deferred {
    using has_z1 = void;
    using has_zn = void;

    template <typename T>
    using z1 = given_one<T>;
    template <typename... Ts>
    using zn = given_pack<Ts...>;
};

// composing continuations
template <typename... Fs>
struct composition final { };

template <typename F, typename... Fs>
struct cont_traits<composition<F, Fs...>> : cont_traits<F> { };

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
    using f_full = decompose<Fs...>::template f<>;

    template <typename... Fs>
    using f_next = recompose<sizeof...(Fs) == 1>::template f<Fs...>;
};

template <typename... Fs>
using compose = compose_impl::template f_full<Fs...>;

template <typename... Fs>
using compose_next = compose_impl::template f_next<Fs...>;

template <initiator C, typename... Cs>
using run = initiate<compose<C, Cs...>>;

// calling continuations with compositions
template <initiator F, typename... Fs>
struct initiate_impl<composition<F, Fs...>> {
    using f0 = F::template f0<compose_next<Fs...>>;
};
template <continues_one F, typename... Fs>
struct call1_impl<composition<F, Fs...>> {
    template <typename T>
    using f1 = F::template f1<compose_next<Fs...>, T>;
};
template <terminates_one F, typename... Fs>
struct call1_impl<composition<F, Fs...>> {
    template <typename T>
    using f1 = F::template z1<T>;
};
template <continues_pack F, typename... Fs>
struct calln_impl<composition<F, Fs...>> {
    template <typename... Ts>
    using fn = F::template fn<compose_next<Fs...>, Ts...>;
};
template <terminates_pack F, typename... Fs>
struct calln_impl<composition<F, Fs...>> {
    template <typename... Ts>
    using fn = F::template zn<Ts...>;
};

// example any-to-same: noop
struct noop {
    using has_f1 = void;
    using has_fn = void;

    template <accepts_one C, typename T>
    using f1 = call1<C, T>;
    template <accepts_pack C, typename... Ts>
    using fn = calln<C, Ts...>;
};

// list
template <typename... Ts>
struct list {
    static constexpr std::size_t size = sizeof...(Ts);
};

struct to_list {
    using has_fn = void;

    template <accepts_one C, typename... Ts>
    using fn = call1<C, list<Ts...>>;
};

struct from_list {
private:
    template <typename T>
    struct from_list_impl {
        static_assert(false, "T is not a list<Ts...>.");
    };

    template <typename... Ts>
    struct from_list_impl<list<Ts...>> {
        template <typename C>
        using f = calln<C, Ts...>;
    };

public:
    using has_f1 = void;

    template <accepts_pack C, typename T>
    using f1 = from_list_impl<T>::template f<C>;
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
        using f = call1<C, Found>;
    };

public:
    using has_fn = void;

    template <accepts_one C, typename... Ts>
        requires(I < sizeof...(Ts))
    using fn = dcall<at_impl<0>, sizeof...(Ts)>::template f<C, Ts...>;
};

// disambiguation: as_one and as_pack may be used as the first item
// in a composition to disambiguate the general call case where only
// one argument is passed in.
struct as_one {
    using has_f1 = void;

    template <accepts_one C, typename T>
    using f1 = call1<C, T>;
};

struct as_pack {
    using has_fn = void;

    template <accepts_pack C, typename... Ts>
    using fn = calln<C, Ts...>;
};

// common template type for errors (I suppose?)
template <typename T>
struct error_t { };

// for debugging: produce a compiler error with errored's input
struct errored {
private:
    template <typename... Ts>
    struct print;

public:
    using has_z1 = void;
    using has_zn = void;

    template <typename T>
    using z1 = typename print<T>::type;
    template <typename... Ts>
    using zn = typename print<Ts...>::type;
};

struct returned {
    using has_z1 = void;
    template <typename T>
    using z1 = T;
};

struct listed {
    using has_zn = void;
    template <typename... Ts>
    using zn = list<Ts...>;
};
}
#include <string>
namespace ctlox {
namespace callztests {
    using t1 = initiate<given_one<int>>;
    using tn0 = calln<deferred>;
    using tn1 = calln<deferred, int>;
    using tn2 = calln<deferred, char, short>;
}

namespace comptests {
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
    struct one_terminal {
        using has_z1 = void;
        template <typename T>
        using z1 = T;
    };
    struct pack_terminal {
        using has_zn = void;
        template <typename... Ts>
        using zn = list<Ts...>;
    };

    // terminated packs
    using t0 = initiate<compose<zero_to_one, one_to_one, one_to_pack, pack_terminal>>;
    static_assert(std::same_as<t0, list<int, int>>);
    using t1 = initiate<compose<zero_to_pack, pack_to_pack, pack_to_one, one_terminal>>;
    static_assert(std::same_as<t1, int>);

    // unterminated packs: result can be used in another pack
    using t2 = initiate<compose<zero_to_pack>>;
    using t3 = initiate<compose<t2, pack_to_one, one_to_one>>;
    using t4 = initiate<compose<t3, one_to_pack, pack_to_pack>>;
}

namespace calltests {
    using namespace comptests;

    using t0 = call<deferred, int, char>;
    using t1 = call<zero_to_one>;
    using t2 = call<one_to_pack, int>;
    using t3 = call<pack_to_one>;
    using t4 = call<pack_to_one, int>;
    using t5 = call<pack_to_pack, int, char>;

    // the following fail (and that's intended):
    // using t6f = call<zero_to_one, char>;
    // using t7f = call<zero_to_pack, int, char>;
    // using t8f = call<one_to_one>;
    // using t9f = call<one_to_one, char, int>;
}

namespace tests {

    // example one-to-pack: repeat_5 (could be repeat<N>)
    struct repeat_5 {
        using has_f1 = void;

        template <accepts_pack C, typename Arg>
        using f1 = calln<C, Arg, Arg, Arg, Arg, Arg>;
    };

    static_assert(std::is_same_v<at<3>, compose<at<3>>>);

    static_assert(std::is_same_v<call1<compose<noop, returned>, double>, double>);

    using my_composition = compose<
        given_pack<int, double, std::string, std::size_t>,
        compose<at<2>, noop>,
        compose<noop, repeat_5>,
        listed>;

    using my_list = call<my_composition>;

    static_assert(std::is_same_v<
        my_list,
        list<std::string, std::string, std::string, std::string, std::string>>);

    using my_comp_2 = compose<
        as_one,
        repeat_5,
        at<3>,
        returned>;

    using my_t = call<my_comp_2, char>;
}
}
