//
// Created by abdullin on 2/11/19.
//

#ifndef BOREALIS_CASTS_HPP
#define BOREALIS_CASTS_HPP

#include "Interpreter/Domain/Numerical/Bound.hpp"
#include "Interpreter/Domain/Numerical/Interval.hpp"
#include "Interpreter/Domain/Numerical/Number.hpp"
#include "Interpreter/Domain/Memory/PointerDomain.hpp"

#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace util {

template <typename F, typename T>
struct cast {
    T operator()(const F&) {
        UNREACHABLE("Unknown cast");
    }
};

using namespace absint;

template <bool s1, bool s2>
struct cast<BitInt<s1>, BitInt<s2>> {
    BitInt<s2> operator()(const BitInt<s1>& n) {
        return BitInt<s2>(n.width(), (size_t) n);
    }
};

template <bool sign>
struct cast<Float, BitInt<sign>> {
    BitInt<sign> operator()(const Float& n) {
        return n.toBitInt<sign>();
    }
};

template <bool sign>
struct cast<BitInt<sign>, Float> {
    Float operator()(const BitInt<sign>& n) {
        return Float((int) (size_t) n);
    }
};

template <typename N1, typename N2>
struct cast<Bound<N1>, Bound<N2>> {
    Bound<N2> operator()(const Bound<N1>& bound) {
        if (bound.isPlusInfinity()) {
            return Bound<N2>::plusInfinity();
        } else if (bound.isMinusInfinity()) {
            return Bound<N2>::minusInfinity();
        } else {
            return Bound<N2>(cast<N1, N2>()(bound.number()));
        }
    }
};

template <typename N1, typename N2>
struct cast<Interval<N1>, Interval<N2>> {
    Interval<N2> operator()(const Interval<N1>& interval) {
        return Interval<N2>(cast<Bound<N1>, Bound<N2>>()(interval.lb()), cast<Bound<N1>, Bound<N2>>()(interval.ub()));
    }
};


template <typename N>
struct convert {
    N operator()(const N&, unsigned int) {
        UNREACHABLE("Unknown cast");
    }
};

template <bool sign>
struct convert<BitInt<sign>> {
    BitInt<sign> operator()(const BitInt<sign>& n, unsigned int to) {
        return n.convert<sign>(to);
    }
};

template <typename N>
struct convert<Bound<N>> {
    Bound<N> operator()(const Bound<N>& bound, unsigned int to) {
        if (bound.isPlusInfinity()) {
            return Bound<N>::plusInfinity();
        } else if (bound.isMinusInfinity()) {
            return Bound<N>::minusInfinity();
        } else {
            return Bound<N>(convert<N>()(bound.number(), to));
        }
    }
};

template <typename N>
struct convert<Interval<N>> {
    Interval<N> operator()(const Interval<N>& interval, unsigned int to) {
        return Interval<N>(convert<Bound<N>>()(interval.lb(), to), convert<Bound<N>>()(interval.ub(), to));
    }
};

}   /* namespace util */
}   /* namespace borealis */

#include "Util/unmacros.h"


#endif //BOREALIS_CASTS_HPP
