//
// Created by abdullin on 2/17/17.
//

#include "DomainFactory.h"
#include "FloatInterval.h"
#include "Interpreter/Util.hpp"
#include "Util/hash.hpp"
#include "Util/sayonara.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

FloatInterval::FloatInterval(DomainFactory* factory, const llvm::APFloat& constant) :
        FloatInterval(factory, constant, constant) {}

FloatInterval::FloatInterval(DomainFactory* factory, const llvm::APFloat& lb, const llvm::APFloat& ub) :
        FloatInterval(factory, std::make_tuple(VALUE, lb, ub)) {}

FloatInterval::FloatInterval(DomainFactory* factory, const FloatInterval::ID& id) :
        Domain(std::get<0>(id), Type::FLOAT_INTERVAL, factory),
        lb_(std::get<1>(id)),
        ub_(std::get<2>(id)),
        wm_(FloatWidening::getInstance()) {
    if (value_ == TOP) setTop();
    else if (lb_.isSmallest() && ub_.isLargest()) value_ = TOP;
}

void FloatInterval::setTop() {
    value_ = TOP;
    lb_ = util::getMinValue(getSemantics());
    ub_ = util::getMaxValue(getSemantics());
}

bool FloatInterval::equals(const Domain* other) const {
    auto&& interval = llvm::dyn_cast<FloatInterval>(other);
    if (not interval) return false;
    if (this == other) return true;

    if (this->isBottom() && interval->isBottom()) return true;
    if (this->isTop() && interval->isTop()) return true;

    return util::eq(this->lb_, interval->lb_) &&
            util::eq(this->ub_, interval->ub_);
}

bool FloatInterval::operator<(const Domain& other) const {
    auto&& interval = llvm::dyn_cast<FloatInterval>(&other);
    ASSERT(interval, "Comparing domains of different type");

    if (other.isBottom()) return false;
    if (this->isBottom()) return true;
    if (this->isTop()) return false;

    auto fromcmp = interval->lb_.compare(this->lb_);
    auto tocmp = this->ub_.compare(interval->ub_);
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
        auto newFrom = util::lt(this->lb_, interval->lb_) ? lb_ : interval->lb_;
        auto newTo = util::lt(this->ub_, interval->ub_) ? interval->ub_ : ub_;
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
        auto left = util::lt(this->lb_, interval->lb_) ? interval->lb_ : lb_;
        auto right = util::lt(this->ub_, interval->ub_) ? ub_ : interval->ub_;
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

    auto left = (util::lt(interval->lb_, lb_)) ? wm_->get_prev(interval->lb_, factory_) : lb_;
    auto right = (util::lt(ub_, interval->ub_)) ? wm_->get_next(interval->ub_, factory_) : ub_;

    return factory_->getFloat(left, right);
}

const llvm::fltSemantics& FloatInterval::getSemantics() const {
    return lb_.getSemantics();
}

llvm::APFloat::roundingMode FloatInterval::getRoundingMode() const {
    return llvm::APFloat::rmNearestTiesToEven;
}

bool FloatInterval::isConstant() const {
    return util::eq(lb_, ub_);
}

const llvm::APFloat& FloatInterval::lb() const {
    return lb_;
}

const llvm::APFloat& FloatInterval::ub() const {
    return ub_;
}

/// Assumes that both intervals have value
bool FloatInterval::hasIntersection(const FloatInterval* other) const {
    ASSERT(this->isValue() && other->isValue(), "Not value intervals");

    if (util::lt(this->lb_, other->lb_) && util::lt(other->lb_, ub_)) {
        return true;
    } else if (util::lt(other->lb_, ub_) && util::lt(ub_, other->ub_)) {
        return true;
    }
    return false;
}

size_t FloatInterval::hashCode() const {
    return util::hash::simple_hash_value(value_, getType(), lb_, ub_);
}

std::string FloatInterval::toPrettyString(const std::string&) const {
    if (isBottom()) return "[]";
    std::ostringstream ss;
    ss << "[" << util::toString(lb_) << ", " << util::toString(ub_) << "]";
    return ss.str();
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
        auto left = lb_;
        auto right = ub_;
        CHECKED_OP(left.add(interval->lb(), getRoundingMode()));
        CHECKED_OP(right.add(interval->ub(), getRoundingMode()));
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
        auto left = lb_;
        auto right = ub_;
        CHECKED_OP(left.subtract(interval->ub(), getRoundingMode()));
        CHECKED_OP(right.subtract(interval->ub(), getRoundingMode()));
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
        auto ll(lb_), ul(ub_), lu(lb_), uu(ub_);
        CHECKED_OP(ll.multiply(interval->lb(), getRoundingMode()));
        CHECKED_OP(ul.multiply(interval->lb(), getRoundingMode()));
        CHECKED_OP(lu.multiply(interval->ub(), getRoundingMode()));
        CHECKED_OP(uu.multiply(interval->ub(), getRoundingMode()));

        auto lb = min(ll, ul, lu, uu);
        auto ub = max(ll, ul, lu, uu);

        return factory_->getFloat(lb, ub);
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

        if (interval->isConstant() && interval->lb_.isZero()) {
            return factory_->getFloat(TOP, getSemantics());
        }

        auto ll(lb_), ul(ub_), lu(lb_), uu(ub_);

        if (interval->lb().isZero()) { //From is zero -> divide by very small number larger than zero -> infinity with same sign
            ll = llvm::APFloat::getInf(ll.getSemantics(), lb_.isNegative());
            ul = llvm::APFloat::getInf(ll.getSemantics(), ub_.isNegative());
        }
        else {
            CHECKED_OP(ll.divide(interval->lb(), getRoundingMode()));
            CHECKED_OP(ul.divide(interval->lb(), getRoundingMode()));
        }

        if (interval->ub().isZero()) { //To is zero -> divide by very small number smaller than zero -> infinity with changed sign
            lu = llvm::APFloat::getInf(ll.getSemantics(), not lb_.isNegative());
            uu = llvm::APFloat::getInf(ll.getSemantics(), not ub_.isNegative());
        }
        else {
            CHECKED_OP(lu.divide(interval->ub(), getRoundingMode()));
            CHECKED_OP(uu.divide(interval->ub(), getRoundingMode()));
        }

        auto lb = min(ll, ul, lu, uu);
        auto ub = max(ll, ul, lu, uu);

        return factory_->getFloat(lb, ub);
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
    auto lb = lb_, ub = ub_;
    lb.convert(newSemantics, getRoundingMode(), &isExact);
    ub.convert(newSemantics, getRoundingMode(), &isExact);
    return factory_->getFloat(lb, ub);
}

Domain::Ptr FloatInterval::fpext(const llvm::Type& type) const {
    ASSERT(type.isFloatingPointTy(), "Non-FP type in fptrunc");
    auto& newSemantics = util::getSemantics(type);

    bool isExact = false;
    auto lb = lb_, ub = ub_;
    lb.convert(newSemantics, getRoundingMode(), &isExact);
    ub.convert(newSemantics, getRoundingMode(), &isExact);
    return factory_->getFloat(lb, ub);
}

Domain::Ptr FloatInterval::fptoui(const llvm::Type& type) const {
    ASSERT(type.isIntegerTy(), "Non-integer type in fptoui");
    auto&& intType = llvm::cast<llvm::IntegerType>(&type);

    bool isExactLB = false, isExactUB = false;
    llvm::APSInt lb(intType->getBitWidth(), true), ub(intType->getBitWidth(), true);

    lb_.convertToInteger(lb, getRoundingMode(), &isExactLB);
    ub_.convertToInteger(ub, getRoundingMode(), &isExactUB);
    return factory_->getInteger(isExactLB ? factory_->toInteger(lb) : Integer::getMinValue(intType->getBitWidth()),
                                isExactUB ? factory_->toInteger(ub) : Integer::getMaxValue(intType->getBitWidth()));
}

Domain::Ptr FloatInterval::fptosi(const llvm::Type& type) const {
    ASSERT(type.isIntegerTy(), "Non-integer type in fptoui");
    auto&& intType = llvm::cast<llvm::IntegerType>(&type);

    bool isExactLB = false, isExactUB = false;
    llvm::APSInt lb(intType->getBitWidth(), false), ub(intType->getBitWidth(), false);

    lb_.convertToInteger(lb, getRoundingMode(), &isExactLB);
    ub_.convertToInteger(ub, getRoundingMode(), &isExactUB);
    return factory_->getInteger(isExactLB ? factory_->toInteger(lb) : Integer::getMinValue(intType->getBitWidth()),
                                isExactUB ? factory_->toInteger(ub) : Integer::getMaxValue(intType->getBitWidth()));
}

Domain::Ptr FloatInterval::bitcast(const llvm::Type& type) const {
    if (type.isIntegerTy()) {
        auto&& intType = llvm::dyn_cast<llvm::IntegerType>(&type);
        auto&& lb = factory_->toInteger(llvm::APInt(intType->getBitWidth(), *lb_.bitcastToAPInt().getRawData()));
        auto&& ub = factory_->toInteger(llvm::APInt(intType->getBitWidth(), *ub_.bitcastToAPInt().getRawData()));
        return factory_->getInteger(lb, ub);

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
        return factory_->getInteger(factory_->toInteger(retval));
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
            else if (hasIntersection(interval))
                return factory_->getInteger(TOP, 1);
            else
                return getBool(false);

        case llvm::CmpInst::FCMP_OGT:   // 0 0 1 0  ordered and greater than
        case llvm::CmpInst::FCMP_UGT:   // 1 0 1 0  unordered or greater than
            res = lb_.compare(interval->ub_);
            if (res == llvm::APFloat::cmpGreaterThan)
                return getBool(true);
            else if (hasIntersection(interval))
                return factory_->getInteger(TOP, 1);
            else
                return getBool(false);

        case llvm::CmpInst::FCMP_OGE:   // 0 0 1 1  ordered and greater than or equal
        case llvm::CmpInst::FCMP_UGE:   // 1 0 1 1  unordered, greater than, or equal
            res = lb_.compare(interval->ub_);
            if (res == llvm::APFloat::cmpGreaterThan || res == llvm::APFloat::cmpEqual)
                return getBool(true);
            else if (hasIntersection(interval))
                return factory_->getInteger(TOP, 1);
            else
                return getBool(false);

        case llvm::CmpInst::FCMP_OLT:   // 0 1 0 0  ordered and less than
        case llvm::CmpInst::FCMP_ULT:   // 1 1 0 0  unordered or less than
            res = lb_.compare(interval->ub_);
            if (res == llvm::APFloat::cmpLessThan)
                return getBool(true);
            else if (hasIntersection(interval))
                return factory_->getInteger(TOP, 1);
            else
                return getBool(false);

        case llvm::CmpInst::FCMP_OLE:   // 0 1 0 1  ordered and less than or equal
        case llvm::CmpInst::FCMP_ULE:   // 1 1 0 1  unordered, less than, or equal
            res = lb_.compare(interval->ub_);
            if (res == llvm::APFloat::cmpLessThan || res == llvm::APFloat::cmpEqual)
                return getBool(true);
            else if (hasIntersection(interval))
                return factory_->getInteger(TOP, 1);
            else
                return getBool(false);

        case llvm::CmpInst::FCMP_ONE:   // 0 1 1 0  ordered and operands are unequal
        case llvm::CmpInst::FCMP_UNE:   // 1 1 1 0  unordered or not equal
            if (not hasIntersection(interval))
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
    return isValue() && lb_.isNaN() && ub_.isNaN();
}

Split FloatInterval::splitByEq(Domain::Ptr other) const {
    auto interval = llvm::dyn_cast<FloatInterval>(other.get());
    ASSERT(interval, "Not interval in split");

    if (this->operator<(*other.get())) return {shared_from_this(), shared_from_this()};

    return interval->isConstant() ?
           Split{ interval->shared_from_this(), shared_from_this() } :
           Split{ shared_from_this(), shared_from_this() };
}

Split FloatInterval::splitByLess(Domain::Ptr other) const {
    auto interval = llvm::dyn_cast<FloatInterval>(other.get());
    ASSERT(interval, "Not interval in split");

    if (this->operator<(*other.get())) return {shared_from_this(), shared_from_this()};

    auto trueVal = factory_->getFloat(lb_, interval->ub_);
    auto falseVal = factory_->getFloat(interval->ub_, this->ub_);
    return {trueVal, falseVal};
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"