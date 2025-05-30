#pragma once

#include <type_traits>
#include <variant>

namespace ctlox::v2 {

template <const auto& v>
struct static_visit {
    static constexpr const auto& operator()() noexcept {
        using T = std::remove_cvref_t<decltype(v)>;
        if constexpr (requires { T::template static_visit<v>(); }) {
            return T::template static_visit<v>();
        } else {
            return std::get<v.index()>(v);
        }
    }

    static constexpr const auto& value = operator()();
};

template <const auto& v>
constexpr const auto& static_visit_v = static_visit<v>::value;

}  // namespace ctlox::v2