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

    template <typename... Ts>
    struct list {
        static constexpr std::size_t size = sizeof...(Ts);
        using type = list<Ts...>;
    };

    // terminal
    struct as_list {
        template <typename... Ts>
        using f = list<Ts...>;
    };

    // terminal
    struct as_error {
        template <typename...> struct as_error_impl;
        template <typename... Ts>
        using f = as_error_impl<Ts...>::type;
    };

    // tereminal
    struct identity {
        template <typename T>
        using f = T;
    };


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
            using f = dcall<C, sizeof(Found)>::template f<Found>;
        };

    public:
        template <typename C, typename... Ts>
            requires (I < sizeof...(Ts))
        using f = dcall<at_impl<0>, sizeof...(Ts)>::template f<C, Ts...>;
    };

    //template <typename F, typename... Fs>
    //struct composed {
    //    template <typename C, typename... Ts>
    //    using f = F::template f<
    //        composed<Fs...>::template f,
    //    >;
    //    //template <typename C, typename... Ts>
    //    //using f = dcall<F, sizeof...(Ts)>::
    //    //    template f<compose_impl<Fs...>, Ts...>;
    //};

    //template <typename F0, typename F1, typename F2>
    //struct composed_3 {
    //    template <typename C, typename... Ts>
    //    using f = F0::template f<
    //        F1::template f<
    //            F2::template f<C
    //};

    //template <typename F>
    //struct composed<F> {
    //    template <typename C, typename... Ts>
    //    using f = dcall<F, sizeof...(Ts)>::template f<C, Ts...>;
    //};

    //template <typename... Fs>
    //struct compose {
    //    template <typename C, typename... Ts>
    //    using f = composed<Fs...>::template f<C, Ts...>;
    //};
}

template <std::size_t I, typename... Ts>
struct std::tuple_element<I, ctlox::list<Ts...>> {
    using type = ctlox::at<I>::template f<ctlox::identity, Ts...>;
};

namespace ctlox::tests {
    static_assert(std::same_as<
        std::tuple_element_t<0, list<int, double>>,
        int>);
    static_assert(std::same_as<
        std::tuple_element_t<1, list<int, double>>,
        double>);
}