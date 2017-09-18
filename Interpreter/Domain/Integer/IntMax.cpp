//
// Created by abdullin on 6/2/17.
//

#include <cstddef>

#include "IntMax.h"
#include "Util/hash.hpp"
#include "Util/sayonara.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

IntMax::IntMax(size_t width) : Integer(MAX, width) {}

bool IntMax::classof(const Integer* other) {
    return other->isMax();
}

std::string IntMax::toString() const {
    return "MAX";
}

std::string IntMax::toSignedString() const {
    return toString();
}

#define MAX_INT_IMPL(func) Integer::Ptr IntMax::func(Integer::Ptr) const { return shared_from_this(); }

MAX_INT_IMPL(add);
MAX_INT_IMPL(sub);
MAX_INT_IMPL(mul);
MAX_INT_IMPL(udiv);
MAX_INT_IMPL(sdiv);
MAX_INT_IMPL(urem);
MAX_INT_IMPL(srem);

bool IntMax::eq(Integer::Ptr other) const { return other->isMax(); }
bool IntMax::lt(Integer::Ptr) const { return false; }
bool IntMax::slt(Integer::Ptr) const { return false; }

Integer::Ptr IntMax::shl(Integer::Ptr) const { return shared_from_this(); }
Integer::Ptr IntMax::lshr(Integer::Ptr) const { return shared_from_this(); }
Integer::Ptr IntMax::ashr(Integer::Ptr) const { return shared_from_this(); }

Integer::Ptr IntMax::trunc(const size_t width) const {
    ASSERT(width <= getWidth(), "Trunc to bigger width");
    return Integer::getMaxValue(width);
}

Integer::Ptr IntMax::zext(const size_t width) const {
    ASSERT(width >= getWidth(), "Ext to smaller width");
    return Integer::getMaxValue(width);
}

Integer::Ptr IntMax::sext(const size_t width) const {
    ASSERT(width >= getWidth(), "Ext to smaller width");
    return Integer::getMaxValue(width);
}

size_t IntMax::hashCode() const {
    return util::hash::simple_hash_value(getWidth());
}

const llvm::APInt& IntMax::getValue() const {
    static auto val = llvm::APInt::getMaxValue(getWidth());
    return val;
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"
