//
// Created by abdullin on 6/2/17.
//

#include "Integer.h"
#include "IntMin.h"
#include "IntMax.h"

namespace borealis {
namespace absint {

Integer::Ptr Integer::getMaxValue(size_t width) {
    return Integer::Ptr{ new IntMax(width)};
}

Integer::Ptr Integer::getMinValue(size_t width) {
    return Integer::Ptr{ new IntMin(width)};
}

bool Integer::neq(Integer::Ptr other) const {
    return not eq(other);
}

bool Integer::le(Integer::Ptr other) const {
    return lt(other) || eq(other);
}

bool Integer::gt(Integer::Ptr other) const {
    return not le(other);
}

bool Integer::ge(Integer::Ptr other) const {
    return gt(other) || eq(other);
}

bool Integer::sle(Integer::Ptr other) const {
    return slt(other) || eq(other);
}

bool Integer::sgt(Integer::Ptr other) const {
    return not sle(other);
}

bool Integer::sge(Integer::Ptr other) const {
    return sgt(other) || eq(other);
}

}   /* namespace absint */
}   /* namespace borealis */