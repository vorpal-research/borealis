//
// Created by abdullin on 6/2/17.
//
#include <cstddef>
#include <Logging/logger.hpp>

#include "Interpreter/Util.h"
#include "MaxInteger.h"
#include "MinInteger.h"
#include "ValInteger.h"
#include "Util/hash.hpp"
#include "Util/sayonara.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

ValInteger::ValInteger(uint64_t val, size_t width) :
        Integer(VALUE, width),
        value_(width, val) {}

ValInteger::ValInteger(const llvm::APInt& value, size_t width) :
        Integer(VALUE, width),
        value_(value) {
    ASSERT(getWidth() == value.getBitWidth(), "Different width in ValInteger args");
}

bool ValInteger::classof(const Integer* other) {
    return other->isValue();
}

std::string ValInteger::toString() const {
    return util::toString(value_);
}

const llvm::APInt& ValInteger::getValue() const {
    return value_;
}

Integer::Ptr ValInteger::add(Integer::Ptr other) const {
    if (not other->isValue()) return other;

    bool isOverflow = false;
    auto newVal = value_.uadd_ov(other->getValue(), isOverflow);
    return isOverflow ?
           nullptr :
           Integer::Ptr{ new ValInteger(newVal, getWidth()) };
}

Integer::Ptr ValInteger::sub(Integer::Ptr other) const {
    if (other->isMin()) return Integer::Ptr{ new MaxInteger(getWidth()) };
    if (other->isMax()) return Integer::Ptr{ new MinInteger(getWidth()) };

    bool isOverflow = false;
    auto newVal = value_.usub_ov(other->getValue(), isOverflow);
    return isOverflow ?
           nullptr :
           Integer::Ptr{ new ValInteger(newVal, getWidth()) };
}

Integer::Ptr ValInteger::mul(Integer::Ptr other) const {
    if (not other->isValue()) return other;

    bool isOverflow = false;
    auto newVal = value_.umul_ov(other->getValue(), isOverflow);
    return isOverflow ?
           nullptr :
           Integer::Ptr{ new ValInteger(newVal, getWidth()) };
}

Integer::Ptr ValInteger::udiv(Integer::Ptr other) const {
    if (other->isMin()) return Integer::Ptr{ new MaxInteger(getWidth()) };
    if (other->isMax()) return Integer::Ptr{ new MinInteger(getWidth()) };
    if (other->getValue() == 0) return Integer::Ptr{ new MaxInteger(getWidth()) };

    return Integer::Ptr{ new ValInteger(value_.udiv(other->getValue()), getWidth()) };
}

Integer::Ptr ValInteger::sdiv(Integer::Ptr other) const {
    if (other->isMin()) return Integer::Ptr{ new MaxInteger(getWidth()) };
    if (other->isMax()) return Integer::Ptr{ new MinInteger(getWidth()) };
    if (other->getValue() == 0) return Integer::Ptr{ new MaxInteger(getWidth()) };

    return Integer::Ptr{ new ValInteger(value_.sdiv(other->getValue()), getWidth()) };
}

Integer::Ptr ValInteger::urem(Integer::Ptr other) const {
    if (not other->isValue()) return other;
    if (other->getValue() == 0) return Integer::Ptr{ new MaxInteger(getWidth()) };

    return Integer::Ptr{ new ValInteger(value_.urem(other->getValue()), getWidth()) };
}

Integer::Ptr ValInteger::srem(Integer::Ptr other) const {
    if (not other->isValue()) return other;
    if (other->getValue() == 0) return Integer::Ptr{ new MaxInteger(getWidth()) };

    return Integer::Ptr{ new ValInteger(value_.srem(other->getValue()), getWidth()) };
}

bool ValInteger::eq(Integer::Ptr other) const {
    if (not other->isValue()) return false;
    if (this->getWidth() != other->getWidth()) return false;

    return this->value_.eq(other->getValue());
}

bool ValInteger::lt(Integer::Ptr other) const {
    if (other->isMin()) return false;
    if (other->isMax()) return true;

    return this->value_.ult(other->getValue());
}

bool ValInteger::slt(Integer::Ptr other) const {
    if (other->isMin()) return false;
    if (other->isMax()) return true;

    return this->value_.slt(other->getValue());
}

Integer::Ptr ValInteger::shl(Integer::Ptr shift) const {
    if (not shift->isValue()) return shift;
    auto shiftVal = shift->getValue();
    return Integer::Ptr{ new ValInteger(value_.shl(shiftVal), getWidth()) };
}

Integer::Ptr ValInteger::lshr(Integer::Ptr shift) const {
    if (not shift->isValue()) return shift;
    auto shiftVal = shift->getValue();
    return Integer::Ptr{ new ValInteger(value_.lshr(shiftVal), getWidth()) };
}

Integer::Ptr ValInteger::ashr(Integer::Ptr shift) const {
    if (not shift->isValue()) return shift;
    auto shiftVal = shift->getValue();
    return Integer::Ptr{ new ValInteger(value_.ashr(shiftVal), getWidth()) };
}

Integer::Ptr ValInteger::trunc(const size_t width) const {
    ASSERT(width <= getWidth(), "Trunc to bigger width");
    return Integer::Ptr{ new ValInteger(value_.trunc(width), width) };
}

Integer::Ptr ValInteger::zext(const size_t width) const {
    ASSERT(width >= getWidth(), "Ext to smaller width");
    return Integer::Ptr{ new ValInteger(value_.zext(width), width) };
}

Integer::Ptr ValInteger::sext(const size_t width) const {
    ASSERT(width >= getWidth(), "Ext to smaller width");
    return Integer::Ptr{ new ValInteger(value_.sext(width), width) };
}

size_t ValInteger::hashCode() const {
    return util::hash::simple_hash_value(getWidth(), value_);
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"

