//
// Created by abdullin on 6/2/17.
//
#include <cstddef>
#include <Logging/logger.hpp>

#include "Interpreter/Util.hpp"
#include "IntMax.h"
#include "IntMin.h"
#include "IntValue.h"
#include "Util/hash.hpp"
#include "Util/sayonara.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

IntValue::IntValue(uint64_t val, size_t width) :
        Integer(VALUE, width),
        value_(width, val) {}

IntValue::IntValue(const llvm::APInt& value, size_t width) :
        Integer(VALUE, width),
        value_(value) {
    ASSERT(getWidth() == value.getBitWidth(), "Different width in IntValue args");
}

bool IntValue::classof(const Integer* other) {
    return other->isValue();
}

std::string IntValue::toString() const {
    return util::toString(value_);
}

std::string IntValue::toSignedString() const {
    return util::toString(value_, true);
}

const llvm::APInt& IntValue::getValue() const {
    return value_;
}

Integer::Ptr IntValue::add(Integer::Ptr other) const {
    if (not other->isValue()) return other;

    bool isOverflow = false;
    auto newVal = value_.uadd_ov(other->getValue(), isOverflow);
    return isOverflow ?
           nullptr :
           Integer::Ptr{ new IntValue(newVal, getWidth()) };
}

Integer::Ptr IntValue::sub(Integer::Ptr other) const {
    if (other->isMin()) return Integer::getMaxValue(getWidth());
    if (other->isMax()) return Integer::getMinValue(getWidth());

    bool isOverflow = false;
    auto newVal = value_.usub_ov(other->getValue(), isOverflow);
    return isOverflow ?
           nullptr :
           Integer::Ptr{ new IntValue(newVal, getWidth()) };
}

Integer::Ptr IntValue::mul(Integer::Ptr other) const {
    if (not other->isValue()) return other;

    bool isOverflow = false;
    auto newVal = value_.umul_ov(other->getValue(), isOverflow);
    return isOverflow ?
           nullptr :
           Integer::Ptr{ new IntValue(newVal, getWidth()) };
}

Integer::Ptr IntValue::udiv(Integer::Ptr other) const {
    if (other->isMin()) return Integer::getMaxValue(getWidth());
    if (other->isMax()) return Integer::getMinValue(getWidth());
    if (other->getValue() == 0) return Integer::getMaxValue(getWidth());

    return Integer::Ptr{ new IntValue(value_.udiv(other->getValue()), getWidth()) };
}

Integer::Ptr IntValue::sdiv(Integer::Ptr other) const {
    if (other->isMin()) return Integer::getMaxValue(getWidth());
    if (other->isMax()) return Integer::getMinValue(getWidth());
    if (other->getValue() == 0) return Integer::getMaxValue(getWidth());

    return Integer::Ptr{ new IntValue(value_.sdiv(other->getValue()), getWidth()) };
}

Integer::Ptr IntValue::urem(Integer::Ptr other) const {
    if (not other->isValue()) return other;
    if (other->getValue() == 0) return Integer::getMaxValue(getWidth());

    return Integer::Ptr{ new IntValue(value_.urem(other->getValue()), getWidth()) };
}

Integer::Ptr IntValue::srem(Integer::Ptr other) const {
    if (not other->isValue()) return other;
    if (other->getValue() == 0) return Integer::getMaxValue(getWidth());

    return Integer::Ptr{ new IntValue(value_.srem(other->getValue()), getWidth()) };
}

bool IntValue::eq(Integer::Ptr other) const {
    if (not other->isValue()) return false;
    if (this->getWidth() != other->getWidth()) return false;

    return this->value_.eq(other->getValue());
}

bool IntValue::lt(Integer::Ptr other) const {
    if (other->isMin()) return false;
    if (other->isMax()) return true;

    return this->value_.ult(other->getValue());
}

bool IntValue::slt(Integer::Ptr other) const {
    if (other->isMin()) return false;
    if (other->isMax()) return true;

    return this->value_.slt(other->getValue());
}

Integer::Ptr IntValue::shl(Integer::Ptr shift) const {
    if (not shift->isValue()) return shift;
    auto shiftVal = shift->getValue();
    return Integer::Ptr{ new IntValue(value_.shl(shiftVal), getWidth()) };
}

Integer::Ptr IntValue::lshr(Integer::Ptr shift) const {
    if (not shift->isValue()) return shift;
    auto shiftVal = shift->getValue();
    return Integer::Ptr{ new IntValue(value_.lshr(shiftVal), getWidth()) };
}

Integer::Ptr IntValue::ashr(Integer::Ptr shift) const {
    if (not shift->isValue()) return shift;
    auto shiftVal = shift->getValue();
    return Integer::Ptr{ new IntValue(value_.ashr(shiftVal), getWidth()) };
}

Integer::Ptr IntValue::trunc(const size_t width) const {
    ASSERT(width <= getWidth(), "Trunc to bigger width");
    return Integer::Ptr{ new IntValue(value_.trunc(width), width) };
}

Integer::Ptr IntValue::zext(const size_t width) const {
    ASSERT(width >= getWidth(), "Ext to smaller width");
    return Integer::Ptr{ new IntValue(value_.zext(width), width) };
}

Integer::Ptr IntValue::sext(const size_t width) const {
    ASSERT(width >= getWidth(), "Ext to smaller width");
    return Integer::Ptr{ new IntValue(value_.sext(width), width) };
}

size_t IntValue::hashCode() const {
    return util::hash::simple_hash_value(getWidth(), value_);
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"

