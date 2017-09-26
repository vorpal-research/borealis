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

IntValue::IntValue(const llvm::APInt& value) :
        Integer(VALUE, value.getBitWidth()),
        value_(value) {}

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
           Integer::getValue(newVal);
}

Integer::Ptr IntValue::sub(Integer::Ptr other) const {
    if (other->isMin()) return Integer::getMaxValue(getWidth());
    if (other->isMax()) return Integer::getMinValue(getWidth());

    bool isOverflow = false;
    auto newVal = value_.usub_ov(other->getValue(), isOverflow);
    return isOverflow ?
           nullptr :
           Integer::getValue(newVal);
}

Integer::Ptr IntValue::mul(Integer::Ptr other) const {
    if (not other->isValue()) return other;

    bool isOverflow = false;
    auto newVal = value_.umul_ov(other->getValue(), isOverflow);
    return isOverflow ?
           nullptr :
           Integer::getValue(newVal);
}

Integer::Ptr IntValue::udiv(Integer::Ptr other) const {
    if (other->isMin()) return Integer::getMaxValue(getWidth());
    if (other->isMax()) return Integer::getMinValue(getWidth());
    if (other->getValue() == 0) return Integer::getMaxValue(getWidth());

    return Integer::getValue(value_.udiv(other->getValue()));
}

Integer::Ptr IntValue::sdiv(Integer::Ptr other) const {
    if (other->isMin()) return Integer::getMaxValue(getWidth());
    if (other->isMax()) return Integer::getMinValue(getWidth());
    if (other->getValue() == 0) return Integer::getMaxValue(getWidth());

    return Integer::getValue(value_.sdiv(other->getValue()));
}

Integer::Ptr IntValue::urem(Integer::Ptr other) const {
    if (not other->isValue()) return other;
    if (other->getValue() == 0) return Integer::getMaxValue(getWidth());

    return Integer::getValue(value_.urem(other->getValue()));
}

Integer::Ptr IntValue::srem(Integer::Ptr other) const {
    if (not other->isValue()) return other;
    if (other->getValue() == 0) return Integer::getMaxValue(getWidth());

    return Integer::getValue(value_.srem(other->getValue()));
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
    return Integer::getValue(value_.shl(shiftVal));
}

Integer::Ptr IntValue::lshr(Integer::Ptr shift) const {
    if (not shift->isValue()) return shift;
    auto shiftVal = shift->getValue();
    return Integer::getValue(value_.lshr(shiftVal));
}

Integer::Ptr IntValue::ashr(Integer::Ptr shift) const {
    if (not shift->isValue()) return shift;
    auto shiftVal = shift->getValue();
    return Integer::getValue(value_.ashr(shiftVal));
}

Integer::Ptr IntValue::trunc(size_t width) const {
    ASSERT(width <= getWidth(), "Trunc to bigger width");
    return Integer::getValue(value_.trunc(width));
}

Integer::Ptr IntValue::zext(size_t width) const {
    ASSERT(width >= getWidth(), "Ext to smaller width");
    return Integer::getValue(value_.zext(width));
}

Integer::Ptr IntValue::sext(size_t width) const {
    ASSERT(width >= getWidth(), "Ext to smaller width");
    return Integer::getValue(value_.sext(width));
}

size_t IntValue::hashCode() const {
    return util::hash::simple_hash_value(getWidth(), value_);
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"

