//
// Created by abdullin on 6/2/17.
//

#include <cstddef>

#include "MaxInteger.h"
#include "Util/hash.hpp"
#include "Util/sayonara.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

MaxInteger::MaxInteger(size_t width) : Integer(MAX, width) {}

bool MaxInteger::classof(const Integer* other) {
    return other->isMax();
}

std::string MaxInteger::toString() const {
    return "MAX";
}

#define MAX_INT_IMPL(func) Integer::Ptr MaxInteger::func(Integer::Ptr) const { return shared_from_this(); }

MAX_INT_IMPL(add);
MAX_INT_IMPL(sub);
MAX_INT_IMPL(mul);
MAX_INT_IMPL(udiv);
MAX_INT_IMPL(sdiv);
MAX_INT_IMPL(urem);
MAX_INT_IMPL(srem);

bool MaxInteger::eq(Integer::Ptr other) const { return other->isMax(); }
bool MaxInteger::lt(Integer::Ptr) const { return false; }
bool MaxInteger::slt(Integer::Ptr) const { return false; }

Integer::Ptr MaxInteger::shl(Integer::Ptr) const { return shared_from_this(); }
Integer::Ptr MaxInteger::lshr(Integer::Ptr) const { return shared_from_this(); }
Integer::Ptr MaxInteger::ashr(Integer::Ptr) const { return shared_from_this(); }

Integer::Ptr MaxInteger::trunc(const size_t width) const {
    ASSERT(width <= getWidth(), "Trunc to bigger width");
    return Integer::Ptr{ new MaxInteger(width) };
}

Integer::Ptr MaxInteger::zext(const size_t width) const {
    ASSERT(width >= getWidth(), "Ext to smaller width");
    return Integer::Ptr{ new MaxInteger(width) };
}

Integer::Ptr MaxInteger::sext(const size_t width) const {
    ASSERT(width >= getWidth(), "Ext to smaller width");
    return Integer::Ptr{ new MaxInteger(width) };
}

size_t MaxInteger::hashCode() const {
    return util::hash::simple_hash_value(getWidth());
}

const llvm::APInt& MaxInteger::getValue() const {
    static auto val = llvm::APInt::getMaxValue(getWidth());
    return val;
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"
