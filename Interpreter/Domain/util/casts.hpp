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

template <bool s1, bool s2>
struct cast<Bound<BitInt<s1>>, Bound<BitInt<s2>>> {
    Bound<BitInt<s2>> operator()(const Bound<BitInt<s1>>& bound) {
        auto* adapter = util::Adapter<BitInt<s2>>::get(bound.caster()->width());
        if (bound.isPlusInfinity()) {
            return Bound<BitInt<s2>>::plusInfinity(adapter);
        } else if (bound.isMinusInfinity()) {
            return Bound<BitInt<s2>>::minusInfinity(adapter);
        } else {
            return Bound<BitInt<s2>>(adapter, cast<BitInt<s1>, BitInt<s2>>()(bound.number()));
        }
    }
};

template <bool s1>
struct cast<Bound<BitInt<s1>>, Bound<Float>> {
    Bound<Float> operator()(const Bound<BitInt<s1>>& bound) {
        auto* adapter = util::Adapter<Float>::get();
        if (bound.isPlusInfinity()) {
            return Bound<Float>::plusInfinity(adapter);
        } else if (bound.isMinusInfinity()) {
            return Bound<Float>::minusInfinity(adapter);
        } else {
            return Bound<Float>(adapter, cast<BitInt<s1>, Float>()(bound.number()));
        }
    }
};

template <bool s2>
struct cast<Bound<Float>, Bound<BitInt<s2>>> {
    Bound<BitInt<s2>> operator()(const Bound<Float>& bound) {
        auto* adapter = util::Adapter<BitInt<s2>>::get(AbstractFactory::defaultSize);
        if (bound.isPlusInfinity()) {
            return Bound<BitInt<s2>>::plusInfinity(adapter);
        } else if (bound.isMinusInfinity()) {
            return Bound<BitInt<s2>>::minusInfinity(adapter);
        } else {
            return Bound<BitInt<s2>>(adapter, cast<Float, BitInt<s2>>()(bound.number()));
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

template <bool sign>
struct convert<Bound<BitInt<sign>>> {
    Bound<BitInt<sign>> operator()(const Bound<BitInt<sign>>& bound, unsigned int to) {
        auto* adapter = util::Adapter<BitInt<sign>>::get(to);
        if (bound.isPlusInfinity()) {
            return Bound<BitInt<sign>>::plusInfinity(adapter);
        } else if (bound.isMinusInfinity()) {
            return Bound<BitInt<sign>>::minusInfinity(adapter);
        } else {
            return Bound<BitInt<sign>>(adapter, convert<BitInt<sign>>()(bound.number(), to));
        }
    }
};

template <typename N>
struct convert<Interval<N>> {
    Interval<N> operator()(const Interval<N>& interval, unsigned int to) {
        auto&& lb = util::convert<Bound<N>>()(interval.lb(), to);
        auto&& ub = util::convert<Bound<N>>()(interval.ub(), to);
        return Interval<N>(lb, ub);
    }
};

}   /* namespace util */
}   /* namespace borealis */

#include "Util/unmacros.h"


#endif //BOREALIS_CASTS_HPP
