#include <ctlox/v1/ct.hpp>
#include <string>

namespace test_v1::test_ct {

using namespace ctlox::v1;

static_assert(std::is_same_v<
    initiate<given_one<int>>,
    given_one<int>>);

static_assert(std::is_same_v<
    calln<deferred>,
    given_pack<>>);

static_assert(std::is_same_v<
    calln<deferred, int>,
    given_pack<int>>);

static_assert(std::is_same_v<
    calln<deferred, char, short>,
    given_pack<char, short>>);

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

static_assert(initiator<compose<zero_to_one>>);
static_assert(std::same_as<
    initiate<compose<compose<compose<zero_to_pack>>>>,
    given_pack<int, char>>);

namespace test_packs {
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

namespace test_call {
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

// example one-to-pack: repeat<N>
template <std::size_t N>
struct repeat {
    template <std::size_t I, typename T>
    struct impl {
        template <typename C, typename... Ts>
        using f = impl<I + 1, T>::template f<C, T, Ts...>;
    };

    template <typename T>
    struct impl<N, T> {
        template <typename C, typename... Ts>
        using f = calln<C, Ts...>;
    };

    using has_f1 = void;
    template <accepts_pack C, typename T>
    using f1 = impl<0, T>::template f<C>;
};

static_assert(std::is_same_v<call1<compose<noop, returned>, double>, double>);

using my_composition = compose<
    given_pack<int, double, std::string, std::size_t>,
    compose<at<2>, noop>,
    compose<noop, repeat<5>>,
    listed>;

static_assert(cont_traits<compose<noop>>::has_fn);
static_assert(cont_traits<compose<compose<noop>>>::has_fn);

using my_list = call<my_composition>;

static_assert(std::is_same_v<
    my_list,
    list<std::string, std::string, std::string, std::string, std::string>>);

using my_comp_2 = compose<
    as_one,
    repeat<5>,
    at<3>,
    returned>;

using my_t = call<my_comp_2, char>;

static_assert(std::is_same_v<
    run<given_pack<char, char, char, int, int>, rotate<3>, listed>,
    list<int, int, char, char, char>>);

using my_tuple_types = run<given_one<int>, repeat<2>, prepend<char>, append<double>>;
using my_tuple = run<my_tuple_types, applied<std::tuple>, returned>;
static_assert(std::is_same_v<my_tuple, std::tuple<char, int, int, double>>);
}
