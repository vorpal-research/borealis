//
// Created by abdullin on 6/2/17.
//

#include <cstddef>

#include "IntMin.h"
#include "Util/hash.hpp"
#include "Util/sayonara.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

IntMin::IntMin(size_t width) : Integer(MIN, width) {}

bool IntMin::classof(const Integer* other) {
    return other->isMin();
}

std::string IntMin::toString() const {
    return "MIN";
}

std::string IntMin::toSignedString() const {
    return toString();
}

#define MAX_INT_IMPL(func) Integer::Ptr IntMin::func(Integer::Ptr) const { return shared_from_this(); }

MAX_INT_IMPL(add);
MAX_INT_IMPL(sub);
MAX_INT_IMPL(mul);
MAX_INT_IMPL(udiv);
MAX_INT_IMPL(sdiv);
MAX_INT_IMPL(urem);
MAX_INT_IMPL(srem);

bool IntMin::eq(Integer::Ptr other) const { return other->isMin(); }
bool IntMin::lt(Integer::Ptr other) const { return not other->isMin(); }
bool IntMin::slt(Integer::Ptr other) const { return not other->isMin(); }

Integer::Ptr IntMin::shl(Integer::Ptr) const { return shared_from_this(); }
Integer::Ptr IntMin::lshr(Integer::Ptr) const { return shared_from_this(); }
Integer::Ptr IntMin::ashr(Integer::Ptr) const { return shared_from_this(); }

Integer::Ptr IntMin::trunc(const size_t width) const {
    ASSERT(width <= getWidth(), "Trunc to bigger width");
    return Integer::Ptr{ new IntMin(width) };
}

Integer::Ptr IntMin::zext(const size_t width) const {
    ASSERT(width >= getWidth(), "Ext to smaller width");
    return Integer::Ptr{ new IntMin(width) };
}

Integer::Ptr IntMin::sext(const size_t width) const {
    ASSERT(width >= getWidth(), "Ext to smaller width");
    return Integer::Ptr{ new IntMin(width) };
}

size_t IntMin::hashCode() const {
    return util::hash::simple_hash_value(getWidth());
}

const llvm::APInt& IntMin::getValue() const {
    static auto val = llvm::APInt::getMinValue(getWidth());
    return val;
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"