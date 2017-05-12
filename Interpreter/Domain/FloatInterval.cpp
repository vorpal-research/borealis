//
// Created by abdullin on 2/17/17.
//

#include "DomainFactory.h"
#include "FloatInterval.h"
#include "Interpreter/Util.h"
#include "Util/hash.hpp"
#include "Util/sayonara.hpp"
#include "Util/streams.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

FloatInterval::FloatInterval(DomainFactory* factory, const llvm::APFloat& constant) :
        FloatInterval(factory, constant, constant) {}

FloatInterval::FloatInterval(Domain::Value value, DomainFactory* factory, const llvm::fltSemantics& semantics) :
        FloatInterval(factory, std::make_tuple(value,
                                               llvm::APFloat(semantics),
                                               llvm::APFloat(semantics))) {}

FloatInterval::FloatInterval(DomainFactory* factory, const llvm::APFloat& from, const llvm::APFloat& to) :
        FloatInterval(factory, std::make_tuple(VALUE, from, to)) {}

FloatInterval::FloatInterval(DomainFactory* factory, const FloatInterval::ID& id) :
        Domain(std::get<0>(id), Type::FLOAT_INTERVAL, factory),
        from_(std::get<1>(id)),
        to_(std::get<2>(id)) {
    if (value_ == TOP) setTop();
    else if (from_.isSmallest() && to_.isLargest()) value_ = TOP;
}

FloatInterval::FloatInterval(const FloatInterval& interval) :
        Domain(interval.value_, Type::FLOAT_INTERVAL, interval.factory_),
        from_(interval.from_),
        to_(interval.to_) {}

void FloatInterval::setTop() {
    value_ = TOP;
    from_ = util::getMinValue(getSemantics());
    to_ = util::getMaxValue(getSemantics());
}

bool FloatInterval::equals(const Domain* other) const {
    auto&& interval = llvm::dyn_cast<FloatInterval>(other);
    if (not interval) return false;
    if (this == other) return true;

    if (this->isBottom() && interval->isBottom()) return true;
    if (this->isTop() && interval->isTop()) return true;

    return util::eq(this->from_, interval->from_) &&
            util::eq(this->to_, interval->to_);
}

bool FloatInterval::operator<(const Domain& other) const {
    auto&& interval = llvm::dyn_cast<FloatInterval>(&other);
    ASSERT(interval, "Comparing domains of different type");

    if (other.isBottom()) return false;
    if (this->isBottom()) return true;
    if (this->isTop()) return false;

    auto fromcmp = interval->from_.compare(this->from_);
    auto tocmp = this->to_.compare(interval->to_);
    return (fromcmp == llvm::APFloat::cmpLessThan || fromcmp == llvm::APFloat::cmpEqual) &&
            (tocmp == llvm::APFloat::cmpLessThan || tocmp == llvm::APFloat::cmpEqual);
}

Domain::Ptr FloatInterval::join(Domain::Ptr other) const {
    auto&& interval = llvm::dyn_cast<FloatInterval>(other.get());
    ASSERT(interval, "Nullptr in interval join");

    if (interval->isBottom()) {
        return shared_from_this();
    } else if (this->isBottom()) {
        return interval->shared_from_this();
    } else {
        auto newFrom = util::lt(this->from_, interval->from_) ? from_ : interval->from_;
        auto newTo = util::lt(this->to_, interval->to_) ? interval->to_ : to_;
        return factory_->getFloat(newFrom, newTo);
    }
}

Domain::Ptr FloatInterval::meet(Domain::Ptr other) const {
    auto&& interval = llvm::dyn_cast<FloatInterval>(other.get());
    ASSERT(interval, "Nullptr in interval meet");

    if (this->isBottom()) {
        return shared_from_this();
    } else if (interval->isBottom()) {
        return interval->shared_from_this();
    } else {
        auto left = util::lt(this->from_, interval->from_) ? interval->from_ : from_;
        auto right = util::lt(this->to_, interval->to_) ? to_ : interval->to_;
        return util::gt(left, right) ?
               shared_from_this() :
               factory_->getFloat(left, right);
    }
}

Domain::Ptr FloatInterval::widen(Domain::Ptr other) const {
    auto&& interval = llvm::dyn_cast<FloatInterval>(other.get());
    ASSERT(interval, "Nullptr in interval");

    if (interval->isBottom()) {
        return shared_from_this();
    } else if (this->isBottom()) {
        return interval->shared_from_this();
    }

    auto left = (util::lt(interval->from_, from_)) ? util::getMinValue(getSemantics()) : from_;
    auto right = (util::lt(to_, interval->to_)) ? util::getMaxValue(getSemantics()) : to_;

    return factory_->getFloat(left, right);
}

Domain::Ptr FloatInterval::narrow(Domain::Ptr other) const {
    auto&& interval = llvm::dyn_cast<FloatInterval>(other.get());
    ASSERT(interval, "Nullptr in interval");

    if (this->isBottom()) {
        return shared_from_this();
    } else if (interval->isBottom()) {
        return interval->shared_from_this();
    }

    auto left = (util::eq(from_, util::getMinValue(getSemantics()))) ? interval->from_ : from_;
    auto right = (util::eq(to_, util::getMaxValue(getSemantics()))) ? interval->to_ : to_;

    return factory_->getFloat(left, right);
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
    ss << "[" << util::toString(from_) << ", " << util::toString(to_) << "]";
    return ss.str();
}

Domain* FloatInterval::clone() const {
    return new FloatInterval(*this);
}

bool FloatInterval::classof(const Domain* other) {
    return other->getType() == Type::FLOAT_INTERVAL;
}

///////////////////////////////////////////////////////////////
/// LLVM IR Semantics
///////////////////////////////////////////////////////////////

#define CHECKED_OP(op) \
if (op != llvm::APFloat::opOK) \
    return factory_->getFloat(TOP, getSemantics());

Domain::Ptr FloatInterval::fadd(Domain::Ptr other) const {
    auto&& interval = llvm::dyn_cast<FloatInterval>(other.get());
    ASSERT(interval, "Nullptr in interval");

    if (this->isBottom() || interval->isBottom()) {
        return factory_->getFloat(getSemantics());
    } else if (this->isTop() || interval->isTop()) {
        return factory_->getFloat(TOP, getSemantics());
    } else {
        auto left = from_;
        auto right = to_;
        CHECKED_OP(left.add(interval->from(), getRoundingMode()));
        CHECKED_OP(right.add(interval->to(), getRoundingMode()));
        return factory_->getFloat(left, right);
    }
}

Domain::Ptr FloatInterval::fsub(Domain::Ptr other) const {
    auto&& interval = llvm::dyn_cast<FloatInterval>(other.get());
    ASSERT(interval, "Nullptr in interval");

    if (this->isBottom() || interval->isBottom()) {
        return factory_->getFloat(getSemantics());
    } else if (this->isTop() || interval->isTop()) {
        return factory_->getFloat(TOP, getSemantics());
    } else {
        auto left = from_;
        auto right = to_;
        CHECKED_OP(left.subtract(interval->to(), getRoundingMode()));
        CHECKED_OP(right.subtract(interval->from(), getRoundingMode()));
        return factory_->getFloat(left, right);
    }
}

Domain::Ptr FloatInterval::fmul(Domain::Ptr other) const {
    auto&& interval = llvm::dyn_cast<FloatInterval>(other.get());
    ASSERT(interval, "Nullptr in interval");

    if (this->isBottom() || interval->isBottom()) {
        return factory_->getFloat(getSemantics());
    } else if (this->isTop() || interval->isTop()) {
        return factory_->getFloat(TOP, getSemantics());
    } else {

        using namespace borealis::util;
        auto fromFrom(from_), toFrom(to_), fromTo(from_), toTo(to_);
        CHECKED_OP(fromFrom.multiply(interval->from(), getRoundingMode()));
        CHECKED_OP(toFrom.multiply(interval->from(), getRoundingMode()));
        CHECKED_OP(fromTo.multiply(interval->to(), getRoundingMode()));
        CHECKED_OP(toTo.multiply(interval->to(), getRoundingMode()));

        auto first = factory_->getFloat(min(fromFrom, toFrom), max(fromFrom, toFrom));
        auto second = factory_->getFloat(min(fromTo, toTo), max(fromTo, toTo));

        return first->join(second);
    }
}

Domain::Ptr FloatInterval::fdiv(Domain::Ptr other) const {
    auto&& interval = llvm::dyn_cast<FloatInterval>(other.get());
    ASSERT(interval, "Nullptr in interval");

    if (this->isBottom() || interval->isBottom()) {
        return factory_->getFloat(getSemantics());
    } else if (this->isTop() || interval->isTop()) {
        return factory_->getFloat(TOP, getSemantics());
    } else {

        using namespace borealis::util;

        if (interval->isConstant() && interval->from_.isZero()) {
            return factory_->getFloat(TOP, getSemantics());
        }

        auto fromFrom(from_), toFrom(to_), fromTo(from_), toTo(to_);

        if (interval->from().isZero()) { //From is zero -> divide by very small number larger than zero -> infinity with same sign
            fromFrom = llvm::APFloat::getInf(fromFrom.getSemantics(), from_.isNegative());
            toFrom = llvm::APFloat::getInf(fromFrom.getSemantics(), to_.isNegative());
        }
        else {
            CHECKED_OP(fromFrom.divide(interval->from(), getRoundingMode()));
            CHECKED_OP(toFrom.divide(interval->from(), getRoundingMode()));
        }

        if (interval->to().isZero()) { //To is zero -> divide by very small number smaller than zero -> infinity with changed sign
            fromTo = llvm::APFloat::getInf(fromFrom.getSemantics(), not from_.isNegative());
            toTo = llvm::APFloat::getInf(fromFrom.getSemantics(), not to_.isNegative());
        }
        else {
            CHECKED_OP(fromTo.divide(interval->to(), getRoundingMode()));
            CHECKED_OP(toTo.divide(interval->to(), getRoundingMode()));
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

    auto&& interval = llvm::dyn_cast<FloatInterval>(other.get());
    ASSERT(interval, "Nullptr in interval");

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
            if (this->isNaN() || interval->isNaN())
                return getBool(false); // Is unordered

            break;
        case llvm::CmpInst::FCMP_ORD:   // 0 1 1 1  ordered (no nans)
            return getBool( not (this->isNaN() || interval->isNaN()) );
        case llvm::CmpInst::FCMP_UNO:   // 1 0 0 0  unordered: isnan(X) | isnan(Y)
            return getBool( (this->isNaN() || interval->isNaN()) );
        case llvm::CmpInst::FCMP_UEQ:   // 1 0 0 1  unordered or equal
        case llvm::CmpInst::FCMP_UGT:   // 1 0 1 0  unordered or greater than
        case llvm::CmpInst::FCMP_UGE:   // 1 0 1 1  unordered, greater than, or equal
        case llvm::CmpInst::FCMP_ULT:   // 1 1 0 0  unordered or less than
        case llvm::CmpInst::FCMP_ULE:   // 1 1 0 1  unordered, less than, or equal
        case llvm::CmpInst::FCMP_UNE:   // 1 1 1 0  unordered or not equal
            if (this->isNaN() || interval->isNaN())
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
            if (isConstant() && this->equals(interval))
                return getBool(true);
            else if (intersects(interval))
                return factory_->getInteger(TOP, 1);
            else
                return getBool(false);

        case llvm::CmpInst::FCMP_OGT:   // 0 0 1 0  ordered and greater than
        case llvm::CmpInst::FCMP_UGT:   // 1 0 1 0  unordered or greater than
            res = from_.compare(interval->to_);
            if (res == llvm::APFloat::cmpGreaterThan)
                return getBool(true);
            else if (intersects(interval))
                return factory_->getInteger(TOP, 1);
            else
                return getBool(false);

        case llvm::CmpInst::FCMP_OGE:   // 0 0 1 1  ordered and greater than or equal
        case llvm::CmpInst::FCMP_UGE:   // 1 0 1 1  unordered, greater than, or equal
            res = from_.compare(interval->to_);
            if (res == llvm::APFloat::cmpGreaterThan || res == llvm::APFloat::cmpEqual)
                return getBool(true);
            else if (intersects(interval))
                return factory_->getInteger(TOP, 1);
            else
                return getBool(false);

        case llvm::CmpInst::FCMP_OLT:   // 0 1 0 0  ordered and less than
        case llvm::CmpInst::FCMP_ULT:   // 1 1 0 0  unordered or less than
            res = from_.compare(interval->to_);
            if (res == llvm::APFloat::cmpLessThan)
                return getBool(true);
            else if (intersects(interval))
                return factory_->getInteger(TOP, 1);
            else
                return getBool(false);

        case llvm::CmpInst::FCMP_OLE:   // 0 1 0 1  ordered and less than or equal
        case llvm::CmpInst::FCMP_ULE:   // 1 1 0 1  unordered, less than, or equal
            res = from_.compare(interval->to_);
            if (res == llvm::APFloat::cmpLessThan || res == llvm::APFloat::cmpEqual)
                return getBool(true);
            else if (intersects(interval))
                return factory_->getInteger(TOP, 1);
            else
                return getBool(false);

        case llvm::CmpInst::FCMP_ONE:   // 0 1 1 0  ordered and operands are unequal
        case llvm::CmpInst::FCMP_UNE:   // 1 1 1 0  unordered or not equal
            if (not intersects(interval))
                return getBool(true);
            else if (not (isConstant() && this->equals(interval)))
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