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

static auto call = [](auto&& f, auto&&... args) QUICK_RETURN(FWD(f)(FWD(args)...));
static auto index = [](auto&& f, auto&& ix) QUICK_RETURN(FWD(f)[FWD(ix)]);

static auto call_begin = [](auto&& c) -> decltype(auto) {
    using std::begin;
    return begin(FWD(c));
};
static auto call_end = [](auto&& c) -> decltype(auto) {
    using std::end;
    return end(FWD(c));
};
static auto call_swap = [](auto&& a, auto&& b) -> decltype(auto) {
    using std::swap;
    swap(FWD(a), FWD(b));
};

} /* namespace ops */
} /* namespace borealis */

#include "Util/unmacros.h"

#endif //_AURORA_SANDBOX_FUNCTIONAL_HPP_
