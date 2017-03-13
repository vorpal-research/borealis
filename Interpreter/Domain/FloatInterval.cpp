//
// Created by abdullin on 2/17/17.
//

#include "DomainFactory.h"
#include "FloatInterval.h"
#include "Util.hpp"
#include "Util/hash.hpp"
#include "Util/sayonara.hpp"
#include "Util/streams.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

FloatInterval::FloatInterval(DomainFactory* factory, const llvm::fltSemantics& semantics) :
        FloatInterval(BOTTOM, factory, semantics) {}

FloatInterval::FloatInterval(DomainFactory* factory, const llvm::APFloat& constant) :
        FloatInterval(factory, constant, constant) {}

FloatInterval::FloatInterval(Domain::Value value, DomainFactory* factory, const llvm::fltSemantics& semantics) :
        FloatInterval(factory, std::make_tuple(value,
                                               llvm::APFloat(semantics),
                                               llvm::APFloat(semantics))) {}

FloatInterval::FloatInterval(DomainFactory* factory, const llvm::APFloat& from, const llvm::APFloat& to) :
        FloatInterval(factory, std::make_tuple(VALUE, from, to)) {}

FloatInterval::FloatInterval(DomainFactory* factory, const FloatInterval::ID& id) :
        Domain(std::get<0>(id), Type::FloatInterval, factory),
        from_(std::get<1>(id)),
        to_(std::get<2>(id)) {
    if (value_ == TOP) setTop();
    else if (from_.isSmallest() && to_.isLargest()) value_ = TOP;
}

FloatInterval::FloatInterval(const FloatInterval& interval) :
        Domain(interval.value_, Type::FloatInterval, interval.factory_),
        from_(interval.from_),
        to_(interval.to_) {}

void FloatInterval::setTop() {
    value_ = TOP;
    from_ = llvm::APFloat::getInf(getSemantics(), true);
    to_ = llvm::APFloat::getInf(getSemantics(), false);
}

bool FloatInterval::equals(const Domain* other) const {
    auto&& value = llvm::dyn_cast<FloatInterval>(other);
    if (not value) return false;

    if (this->isBottom() && value->isBottom()) return true;
    if (this->isTop() && value->isTop()) return true;

    return util::eq(this->from_, value->from_) &&
            util::eq(this->to_, value->to_);
}

bool FloatInterval::operator<(const Domain& other) const {
    auto&& value = llvm::dyn_cast<FloatInterval>(&other);
    ASSERT(value, "Comparing domains of different type");

    if (other.isBottom()) return false;
    if (this->isBottom()) return true;
    if (this->isTop()) return false;

    auto fromcmp = value->from_.compare(this->from_);
    auto tocmp = this->to_.compare(value->to_);
    return (fromcmp == llvm::APFloat::cmpLessThan || fromcmp == llvm::APFloat::cmpEqual) &&
            (tocmp == llvm::APFloat::cmpLessThan || tocmp == llvm::APFloat::cmpEqual);
}

Domain::Ptr FloatInterval::join(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<FloatInterval>(other.get());
    ASSERT(value, "Nullptr in interval join");

    if (value->isBottom()) {
        return shared_from_this();
    } else if (this->isBottom()) {
        return value->shared_from_this();
    } else {
        auto newFrom = util::lt(this->from_, value->from_) ? from_ : value->from_;
        auto newTo = util::lt(this->to_, value->to_) ? value->to_ : to_;
        return factory_->getFloat(newFrom, newTo);
    }
}

Domain::Ptr FloatInterval::meet(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<FloatInterval>(other.get());
    ASSERT(value, "Nullptr in interval meet");

    if (this->isBottom() || value->isBottom()) {
        return clone()->shared_from_this();
    } else {
        auto left = util::lt(this->from_, value->from_) ? value->from_ : from_;
        auto right = util::lt(this->to_, value->to_) ? to_ : value->to_;
        return util::gt(left, right) ?
               clone()->shared_from_this() :
               factory_->getFloat(left, right);
    }
}

Domain::Ptr FloatInterval::widen(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<FloatInterval>(other.get());
    ASSERT(value, "Nullptr in interval");

    if (value->isBottom()) {
        return shared_from_this();
    } else if (this->isBottom()) {
        return value->shared_from_this();
    } else if (this->isTop() || value->isTop()) {
        return factory_->getFloat(TOP, getSemantics());
    } else if (this->equals(value)) {
        return shared_from_this();
    }

    if (this->operator<(*value)) {
        return value->shared_from_this();
    } else if (value->operator<(*this)) {
        return shared_from_this();
    } else if (util::lt(to_, value->from_)) {
        auto left = from_;
        auto right = llvm::APFloat::getInf(getSemantics());
        return factory_->getFloat(left, right);
    } else if (util::lt(value->to_, from_)) {
        auto left = llvm::APFloat::getInf(getSemantics(), true);
        auto right = to_;
        return factory_->getFloat(left, right);
    }

    return factory_->getFloat(TOP, getSemantics());
}

const llvm::fltSemantics& FloatInterval::getSemantics() const {
    return from_.getSemantics();
}

llvm::APFloat::roundingMode FloatInterval::getRoundingMode() const {
    return llvm::APFloat::rmNearestTiesToEven;
}

bool FloatInterval::isConstant() const {
    return util::eq(from_, to_);
}

const llvm::APFloat& FloatInterval::from() const {
    return from_;
}

const llvm::APFloat& FloatInterval::to() const {
    return to_;
}

/// Assumes that both intervals have value
bool FloatInterval::intersects(const FloatInterval* other) const {
    ASSERT(this->isValue() && other->isValue(), "Not value intervals");

    if (util::lt(this->from_, other->from_) && util::lt(other->from_, to_)) {
        return true;
    } else if (util::lt(other->from_, to_) && util::lt(to_, other->to_)) {
        return true;
    }
    return false;
}

size_t FloatInterval::hashCode() const {
    return util::hash::simple_hash_value(value_, getType(),
                                         from_, to_);
}

std::string FloatInterval::toString() const {
    if (isBottom()) return "[]";
    std::ostringstream ss;
    llvm::SmallVector<char, 32> from, to;
    from_.toString(from, 8);
    to_.toString(to, 8);
    ss << "[";
    for (auto&& it : from)
        ss << it;
    ss << ", ";
    for (auto&& it : to)
        ss << it;
    ss << "]";
    return ss.str();
}

Domain* FloatInterval::clone() const {
    return new FloatInterval(*this);
}

bool FloatInterval::classof(const Domain* other) {
    return other->getType() == Type::FloatInterval;
}

#define CHECKED_OP(op) \
if (op != llvm::APFloat::opOK) \
    return factory_->getFloat(TOP, getSemantics());

Domain::Ptr FloatInterval::fadd(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<FloatInterval>(other.get());
    ASSERT(value, "Nullptr in interval");

    if (this->isBottom() || value->isBottom()) {
        return factory_->getFloat(getSemantics());
    } else if (this->isTop() || value->isTop()) {
        return factory_->getFloat(TOP, getSemantics());
    } else {
        auto left = from_;
        auto right = to_;
        CHECKED_OP(left.add(value->from(), getRoundingMode()));
        CHECKED_OP(right.add(value->to(), getRoundingMode()));
        return factory_->getFloat(left, right);
    }
}

Domain::Ptr FloatInterval::fsub(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<FloatInterval>(other.get());
    ASSERT(value, "Nullptr in interval");

    if (this->isBottom() || value->isBottom()) {
        return factory_->getFloat(getSemantics());
    } else if (this->isTop() || value->isTop()) {
        return factory_->getFloat(TOP, getSemantics());
    } else {
        auto left = from_;
        auto right = to_;
        CHECKED_OP(left.subtract(value->to(), getRoundingMode()));
        CHECKED_OP(right.subtract(value->from(), getRoundingMode()));
        return factory_->getFloat(left, right);
    }
}

Domain::Ptr FloatInterval::fmul(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<FloatInterval>(other.get());
    ASSERT(value, "Nullptr in interval");

    if (this->isBottom() || value->isBottom()) {
        return factory_->getFloat(getSemantics());
    } else if (this->isTop() || value->isTop()) {
        return factory_->getFloat(TOP, getSemantics());
    } else {

        using namespace borealis::util;
        auto fromFrom(from_), toFrom(to_), fromTo(from_), toTo(to_);
        CHECKED_OP(fromFrom.multiply(value->from(), getRoundingMode()));
        CHECKED_OP(toFrom.multiply(value->from(), getRoundingMode()));
        CHECKED_OP(fromTo.multiply(value->to(), getRoundingMode()));
        CHECKED_OP(toTo.multiply(value->to(), getRoundingMode()));

        auto first = factory_->getFloat(min(fromFrom, toFrom), max(fromFrom, toFrom));
        auto second = factory_->getFloat(min(fromTo, toTo), max(fromTo, toTo));

        return first->join(second);
    }
}

Domain::Ptr FloatInterval::fdiv(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<FloatInterval>(other.get());
    ASSERT(value, "Nullptr in interval");

    if (this->isBottom() || value->isBottom()) {
        return factory_->getFloat(getSemantics());
    } else if (this->isTop() || value->isTop()) {
        return factory_->getFloat(TOP, getSemantics());
    } else {

        using namespace borealis::util;

        if (value->isConstant() && value->from_.isZero()) {
            return factory_->getFloat(TOP, getSemantics());
        }

        auto fromFrom(from_), toFrom(to_), fromTo(from_), toTo(to_);

        if (value->from().isZero()) { //From is zero -> divide by very small number larger than zero -> infinity with same sign
            fromFrom = llvm::APFloat::getInf(fromFrom.getSemantics(), from_.isNegative());
            toFrom = llvm::APFloat::getInf(fromFrom.getSemantics(), to_.isNegative());
        }
        else {
            CHECKED_OP(fromFrom.divide(value->from(), getRoundingMode()));
            CHECKED_OP(toFrom.divide(value->from(), getRoundingMode()));
        }

        if (value->to().isZero()) { //To is zero -> divide by very small number smaller than zero -> infinity with changed sign
            fromTo = llvm::APFloat::getInf(fromFrom.getSemantics(), not from_.isNegative());
            toTo = llvm::APFloat::getInf(fromFrom.getSemantics(), not to_.isNegative());
        }
        else {
            CHECKED_OP(fromTo.divide(value->to(), getRoundingMode()));
            CHECKED_OP(toTo.divide(value->to(), getRoundingMode()));
        }

        auto first = factory_->getFloat(min(fromFrom, toFrom), max(fromFrom, toFrom));
        auto second = factory_->getFloat(min(fromTo, toTo), max(fromTo, toTo));

        return first->join(second);
    }
}

Domain::Ptr FloatInterval::frem(Domain::Ptr other) const {
    if (this->isBottom() || other->isBottom())
        return factory_->getFloat(getSemantics());

    return factory_->getFloat(TOP, getSemantics());
}

Domain::Ptr FloatInterval::fptrunc(const llvm::Type& type) const {
    ASSERT(type.isFloatingPointTy(), "Non-FP type in fptrunc");
    auto& newSemantics = util::getSemantics(type);

    bool isExact = false;
    auto from = from_, to = to_;
    from.convert(newSemantics, getRoundingMode(), &isExact);
    to.convert(newSemantics, getRoundingMode(), &isExact);
    return factory_->getFloat(from, to);
}

Domain::Ptr FloatInterval::fpext(const llvm::Type& type) const {
    ASSERT(type.isFloatingPointTy(), "Non-FP type in fptrunc");
    auto& newSemantics = util::getSemantics(type);

    bool isExact = false;
    auto from = from_, to = to_;
    from.convert(newSemantics, getRoundingMode(), &isExact);
    to.convert(newSemantics, getRoundingMode(), &isExact);
    return factory_->getFloat(from, to);
}

Domain::Ptr FloatInterval::fptoui(const llvm::Type& type) const {
    ASSERT(type.isIntegerTy(), "Non-integer type in fptoui");
    auto&& intType = llvm::cast<llvm::IntegerType>(&type);

    bool isExact = false;
    llvm::APSInt from(intType->getBitWidth(), true), to(intType->getBitWidth(), true);

    from_.convertToInteger(from, getRoundingMode(), &isExact);
    to_.convertToInteger(to, getRoundingMode(), &isExact);
    return factory_->getInteger(from, to);
}

Domain::Ptr FloatInterval::fptosi(const llvm::Type& type) const {
    ASSERT(type.isIntegerTy(), "Non-integer type in fptoui");
    auto&& intType = llvm::cast<llvm::IntegerType>(&type);

    bool isExact = false;
    llvm::APSInt from(intType->getBitWidth(), false), to(intType->getBitWidth(), false);

    from_.convertToInteger(from, getRoundingMode(), &isExact);
    to_.convertToInteger(to, getRoundingMode(), &isExact);
    return factory_->getInteger(from, to, true);
}

Domain::Ptr FloatInterval::bitcast(const llvm::Type& type) const {
    if (type.isIntegerTy()) {
        auto&& intType = llvm::dyn_cast<llvm::IntegerType>(&type);
        auto&& from = llvm::APInt(intType->getBitWidth(), *from_.bitcastToAPInt().getRawData());
        auto&& to = llvm::APInt(intType->getBitWidth(), *to_.bitcastToAPInt().getRawData());
        return factory_->getInteger(from, to);

    } else if (type.isFloatingPointTy()) {
        return shared_from_this();
        
    } else {
        UNREACHABLE("Bitcast to unknown type");
    }
}

Domain::Ptr FloatInterval::fcmp(Domain::Ptr other, llvm::CmpInst::Predicate operation) const {
    auto&& getBool = [&] (bool val) -> Domain::Ptr {
        llvm::APSInt retval(1, true);
        if (val) retval = 1;
        else retval = 0;
        return factory_->getInteger(retval);
    };

    if (this->isBottom() || other->isBottom()) {
        return factory_->getInteger(1);
    } else if (this->isTop() || other->isTop()) {
        return factory_->getInteger(TOP, 1);
    }

    auto&& value = llvm::dyn_cast<FloatInterval>(other.get());
    ASSERT(value, "Nullptr in interval");

    switch (operation)
    {
        //   Opcode                        U L G E  Intuitive operation
        case llvm::CmpInst::FCMP_FALSE: // 0 0 0 0  always false (always folded)
            return getBool(false);
        case llvm::CmpInst::FCMP_OEQ:   // 0 0 0 1  ordered and equal
        case llvm::CmpInst::FCMP_OGT:   // 0 0 1 0  ordered and greater than
        case llvm::CmpInst::FCMP_OGE:   // 0 0 1 1  ordered and greater than or equal
        case llvm::CmpInst::FCMP_OLT:   // 0 1 0 0  ordered and less than
        case llvm::CmpInst::FCMP_OLE:   // 0 1 0 1  ordered and less than or equal
        case llvm::CmpInst::FCMP_ONE:   // 0 1 1 0  ordered and operands are unequal
            if (this->isNaN() || value->isNaN())
                return getBool(false); // Is unordered

            break;
        case llvm::CmpInst::FCMP_ORD:   // 0 1 1 1  ordered (no nans)
            return getBool( not (this->isNaN() || value->isNaN()) );
        case llvm::CmpInst::FCMP_UNO:   // 1 0 0 0  unordered: isnan(X) | isnan(Y)
            return getBool( (this->isNaN() || value->isNaN()) );
        case llvm::CmpInst::FCMP_UEQ:   // 1 0 0 1  unordered or equal
        case llvm::CmpInst::FCMP_UGT:   // 1 0 1 0  unordered or greater than
        case llvm::CmpInst::FCMP_UGE:   // 1 0 1 1  unordered, greater than, or equal
        case llvm::CmpInst::FCMP_ULT:   // 1 1 0 0  unordered or less than
        case llvm::CmpInst::FCMP_ULE:   // 1 1 0 1  unordered, less than, or equal
        case llvm::CmpInst::FCMP_UNE:   // 1 1 1 0  unordered or not equal
            if (this->isNaN() || value->isNaN())
                return getBool(true); // Is unordered

            break;
        case llvm::CmpInst::FCMP_TRUE:  // 1 1 1 1  always true (always folded)
            return getBool(true);
        default:
            UNREACHABLE("Unknown operation in fcmp");
    }

    llvm::APFloat::cmpResult res;
    switch (operation) {
        case llvm::CmpInst::FCMP_OEQ:   // 0 0 0 1  ordered and equal
        case llvm::CmpInst::FCMP_UEQ:   // 1 0 0 1  unordered or equal
            if (isConstant() && this->equals(value))
                return getBool(true);
            else if (intersects(value))
                return factory_->getInteger(TOP, 1);
            else
                return getBool(false);

        case llvm::CmpInst::FCMP_OGT:   // 0 0 1 0  ordered and greater than
        case llvm::CmpInst::FCMP_UGT:   // 1 0 1 0  unordered or greater than
            res = from_.compare(value->to_);
            if (res == llvm::APFloat::cmpGreaterThan)
                return getBool(true);
            else if (intersects(value))
                return factory_->getInteger(TOP, 1);
            else
                return getBool(false);

        case llvm::CmpInst::FCMP_OGE:   // 0 0 1 1  ordered and greater than or equal
        case llvm::CmpInst::FCMP_UGE:   // 1 0 1 1  unordered, greater than, or equal
            res = from_.compare(value->to_);
            if (res == llvm::APFloat::cmpGreaterThan || res == llvm::APFloat::cmpEqual)
                return getBool(true);
            else if (intersects(value))
                return factory_->getInteger(TOP, 1);
            else
                return getBool(false);

        case llvm::CmpInst::FCMP_OLT:   // 0 1 0 0  ordered and less than
        case llvm::CmpInst::FCMP_ULT:   // 1 1 0 0  unordered or less than
            res = from_.compare(value->to_);
            if (res == llvm::APFloat::cmpLessThan)
                return getBool(true);
            else if (intersects(value))
                return factory_->getInteger(TOP, 1);
            else
                return getBool(false);

        case llvm::CmpInst::FCMP_OLE:   // 0 1 0 1  ordered and less than or equal
        case llvm::CmpInst::FCMP_ULE:   // 1 1 0 1  unordered, less than, or equal
            res = from_.compare(value->to_);
            if (res == llvm::APFloat::cmpLessThan || res == llvm::APFloat::cmpEqual)
                return getBool(true);
            else if (intersects(value))
                return factory_->getInteger(TOP, 1);
            else
                return getBool(false);

        case llvm::CmpInst::FCMP_ONE:   // 0 1 1 0  ordered and operands are unequal
        case llvm::CmpInst::FCMP_UNE:   // 1 1 1 0  unordered or not equal
            if (not intersects(value))
                return getBool(true);
            else if (not (isConstant() && this->equals(value)))
                return factory_->getInteger(TOP, 1);
            else
                return getBool(false);

        default:
            UNREACHABLE("Unknown operation in fcmp");
    }
}

bool FloatInterval::isNaN() const {
    return isValue() && from_.isNaN() && to_.isNaN();
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"