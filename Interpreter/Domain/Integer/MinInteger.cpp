//
// Created by abdullin on 6/2/17.
//

#include <cstddef>

#include "MinInteger.h"
#include "Util/hash.hpp"
#include "Util/sayonara.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

MinInteger::MinInteger(size_t width) : Integer(MIN, width) {}

bool MinInteger::classof(const Integer* other) {
    return other->isMin();
}

std::string MinInteger::toString() const {
    return "MIN";
}

#define MAX_INT_IMPL(func) Integer::Ptr MinInteger::func(Integer::Ptr) const { return shared_from_this(); }

MAX_INT_IMPL(add);
MAX_INT_IMPL(sub);
MAX_INT_IMPL(mul);
MAX_INT_IMPL(udiv);
MAX_INT_IMPL(sdiv);
MAX_INT_IMPL(urem);
MAX_INT_IMPL(srem);

bool MinInteger::eq(Integer::Ptr other) const { return other->isMin(); }
bool MinInteger::lt(Integer::Ptr other) const { return not other->isMin(); }
bool MinInteger::slt(Integer::Ptr other) const { return not other->isMin(); }

Integer::Ptr MinInteger::shl(Integer::Ptr) const { return shared_from_this(); }
Integer::Ptr MinInteger::lshr(Integer::Ptr) const { return shared_from_this(); }
Integer::Ptr MinInteger::ashr(Integer::Ptr) const { return shared_from_this(); }

Integer::Ptr MinInteger::trunc(const size_t width) const {
    ASSERT(width <= getWidth(), "Trunc to bigger width");
    return Integer::Ptr{ new MinInteger(width) };
}

Integer::Ptr MinInteger::zext(const size_t width) const {
    ASSERT(width >= getWidth(), "Ext to smaller width");
    return Integer::Ptr{ new MinInteger(width) };
}

Integer::Ptr MinInteger::sext(const size_t width) const {
    ASSERT(width >= getWidth(), "Ext to smaller width");
    return Integer::Ptr{ new MinInteger(width) };
}

size_t MinInteger::hashCode() const {
    return util::hash::simple_hash_value(getWidth());
}

const llvm::APInt& MinInteger::getValue() const {
    static auto val = llvm::APInt::getMinValue(getWidth());
    return val;
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"