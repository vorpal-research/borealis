#ifndef FUNCTIONAL_HPP_
#define FUNCTIONAL_HPP_

#include <utility>
#include <iterator>

#include "Util/macros.h"

namespace borealis {
namespace ops {


#define DEFINE_UOP(OPNAME, OP) static auto OPNAME = [](auto&& it) QUICK_RETURN(OP(FWD(it)));

DEFINE_UOP(unary_plus, +);
DEFINE_UOP(unary_negate, -);
DEFINE_UOP(logical_not, !);
DEFINE_UOP(bit_not, ~);
DEFINE_UOP(dereference, *);
DEFINE_UOP(take_pointer, &);

DEFINE_UOP(pre_increment, ++);
DEFINE_UOP(pre_decrement, --);

#undef DEFINE_UOP

static auto post_increment = [](auto&& it) QUICK_RETURN((FWD(it))++);
static auto post_decrement = [](auto&& it) QUICK_RETURN((FWD(it))--);

#define DEFINE_BOP(OPNAME, OP) static auto OPNAME = [](auto&& a, auto&& b) QUICK_RETURN(FWD(a) OP FWD(b));
DEFINE_BOP(plus, +);
DEFINE_BOP(minus, -);
DEFINE_BOP(multiplies, *);
DEFINE_BOP(divides, /);
DEFINE_BOP(modulus, %);
DEFINE_BOP(logical_and, &&);
DEFINE_BOP(logical_or, ||);
DEFINE_BOP(bit_and, &);
DEFINE_BOP(bit_or, |);
DEFINE_BOP(bit_xor, ^);
DEFINE_BOP(equals_to, ==);
DEFINE_BOP(not_equals_to, !=);
DEFINE_BOP(less, <);
DEFINE_BOP(less_equal, <=);
DEFINE_BOP(greater, >);
DEFINE_BOP(greater_equal, >=);
DEFINE_BOP(lshift, <<);
DEFINE_BOP(rshift, >>);
#define COMMA ,
DEFINE_BOP(comma, COMMA);
#undef COMMA
#undef DEFINE_BOP

static auto deref_equals_to = [](auto&& a, auto&& b) QUICK_RETURN(*FWD(a) == *FWD(b));

static auto call = [](auto&& f, auto&&... args) QUICK_RETURN(FWD(f)(FWD(args)...));
static auto index = [](auto&& f, auto&& ix) QUICK_RETURN(FWD(f)[FWD(ix)]);

static auto call_begin = [](auto&& c) -> DECLTYPE_AUTO {
    using std::begin;
    return begin(FWD(c));
};
static auto call_end = [](auto&& c) -> DECLTYPE_AUTO {
    using std::end;
    return end(FWD(c));
};
static auto call_swap = [](auto&& a, auto&& b) -> DECLTYPE_AUTO {
    using std::swap;
    swap(FWD(a), FWD(b));
};

template<class Dst>
auto static_caster() {
    return [](auto&& c) -> DECLTYPE_AUTO {
        return static_cast<Dst>(FWD(c));
    };
}

} /* namespace ops */

static auto konst = [](auto&& a) {
    return [&a](auto&&...) QUICK_RETURN(a);
};



namespace impl_ {
template<class Tuple, class Callable, size_t ...N>
auto apply_packed_step_1(Tuple&& tp, Callable c, std::index_sequence<N...>)
QUICK_RETURN(c(std::get<N>(tp)...))

// apply a function taking N parameters to an N-tuple
template<class Callable, class Tuple>
auto apply_packed(Callable c, Tuple&& tp)
QUICK_RETURN(apply_packed_step_1(std::forward<Tuple>(tp), c, std::make_index_sequence<std::tuple_size<std::remove_reference_t<Tuple>>::value>{}))
} // namespace impl

template<class F>
static auto as_packed(F f) {
    return [f](auto&& tuple) -> DECLTYPE_AUTO {
        return impl_::apply_packed(f, std::forward<decltype(tuple)>(tuple));
    };
}

} /* namespace borealis */

#include "Util/unmacros.h"

#endif //_AURORA_SANDBOX_FUNCTIONAL_HPP_
