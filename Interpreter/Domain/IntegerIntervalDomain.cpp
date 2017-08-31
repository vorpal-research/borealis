//
// Created by abdullin on 2/2/17.
//

#include "DomainFactory.h"
#include "IntegerIntervalDomain.h"
#include "Util/sayonara.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

IntegerIntervalDomain::IntegerIntervalDomain(DomainFactory* factory, Integer::Ptr constant) :
        IntegerIntervalDomain(factory, constant, constant) {}

IntegerIntervalDomain::IntegerIntervalDomain(DomainFactory* factory, Integer::Ptr lb, Integer::Ptr ub) :
        IntegerIntervalDomain(factory, std::make_tuple(VALUE,
                                                 lb,
                                                 ub)) {}

IntegerIntervalDomain::IntegerIntervalDomain(DomainFactory* factory, const IntegerIntervalDomain::ID& key) :
        Domain(std::get<0>(key), Type::INTEGER_INTERVAL, factory),
        lb_(value_ == TOP ?
            Integer::getMinValue(std::get<1>(key)->getWidth()) :
            std::get<1>(key)),
        ub_(value_ == TOP ?
            Integer::getMaxValue(std::get<1>(key)->getWidth()) :
            std::get<2>(key)),
        wm_(IntegerWidening::getInstance()) {
    ASSERT(lb_->getWidth() == ub_->getWidth(), "Different bit width of interval bounds");
    ASSERT(lb_->le(ub_), "Lower bound is greater that upper bound");
    if (lb_->isMin() && ub_->isMax()) value_ = TOP;
}

Domain::Ptr IntegerIntervalDomain::join(Domain::Ptr other) {
    auto&& interval = llvm::dyn_cast<IntegerIntervalDomain>(other.get());
    ASSERT(interval, "Nullptr in interval join");
    ASSERT(this->getWidth() == interval->getWidth(), "Joining two intervals of different format");

    if (interval->isBottom()) {
        return shared_from_this();
    } else if (this->isBottom()) {
        return interval->shared_from_this();
    } else {
        auto lb = util::min(lb_, interval->lb_);
        auto ub = util::max(ub_, interval->ub_);
        return factory_->getInteger(lb, ub);
    }
}

Domain::Ptr IntegerIntervalDomain::meet(Domain::Ptr other) {
    auto&& interval = llvm::dyn_cast<IntegerIntervalDomain>(other.get());
    ASSERT(interval, "Nullptr in interval meet");
    ASSERT(this->getWidth() == interval->getWidth(), "Joining two intervals of different format");

    if (this->isBottom()) {
        return shared_from_this();
    } else if (interval->isBottom()) {
        return interval->shared_from_this();
    } else {
        auto lb = lb_->lt(interval->lb_) ? interval->lb_ : lb_;
        auto ub = ub_->lt(interval->ub_) ? ub_ : interval->ub_;

        return lb->gt(ub) ?
               shared_from_this() :
               factory_->getInteger(lb, ub);
    }
}

Domain::Ptr IntegerIntervalDomain::widen(Domain::Ptr other) {
    auto&& interval = llvm::dyn_cast<IntegerIntervalDomain>(other.get());
    ASSERT(interval, "Nullptr in interval");
    ASSERT(this->getWidth() == interval->getWidth(), "Widening two intervals of different format");

    if (interval->isBottom()) {
        return shared_from_this();
    } else if (this->isBottom()) {
        return interval->shared_from_this();
    }

    auto lb = interval->lb_->lt(lb_) ? wm_->get_prev(interval->lb_, factory_) : lb_;
    auto ub = ub_->lt(interval->ub_) ? wm_->get_next(interval->ub_, factory_) : ub_;

    return factory_->getInteger(lb, ub);
}

size_t IntegerIntervalDomain::getWidth() const { return lb_->getWidth(); }
bool IntegerIntervalDomain::isConstant() const { return lb_->eq(ub_); }
bool IntegerIntervalDomain::isConstant(uint64_t constant) const {
    auto constInteger = factory_->toInteger(constant, getWidth());
    return isConstant() && lb_->eq(constInteger);
}
Integer::Ptr IntegerIntervalDomain::lb() const { return lb_; }
Integer::Ptr IntegerIntervalDomain::ub() const { return ub_; }
Integer::Ptr IntegerIntervalDomain::signed_lb() const { return util::min<true>(lb_, ub_); }
Integer::Ptr IntegerIntervalDomain::signed_ub() const { return util::max<true>(lb_, ub_); }

/// Assumes that both intervals have value
bool IntegerIntervalDomain::hasIntersection(const IntegerIntervalDomain* other) const {
    ASSERT(this->isValue() && other->isValue(), "Not value intervals");

    if (lb_->le(other->lb_) && other->lb_->le(ub_)) {
        return true;
    } else if (other->lb_->le(ub_) &&  ub_->le(other->ub_)) {
        return true;
    }
    return false;
}

bool IntegerIntervalDomain::hasIntersection(Integer::Ptr constant) const {
    return lb_->le(constant) && constant->le(ub_);
}

bool IntegerIntervalDomain::equals(const Domain *other) const {
    auto&& interval = llvm::dyn_cast<IntegerIntervalDomain>(other);
    if (not interval) return false;
    if (this == other) return true;

    if (this->isBottom() && interval->isBottom()) {
        return this->getWidth() == interval->getWidth();
    }
    if (this->isTop() && interval->isTop()) {
        return this->getWidth() == interval->getWidth();
    }

    return  this->value_ == interval->value_ &&
            this->getWidth() == interval->getWidth() &&
            lb_->eq(interval->lb_) &&
            ub_->eq(interval->ub_);
}

bool IntegerIntervalDomain::operator<(const Domain &other) const {
    auto&& interval = llvm::dyn_cast<IntegerIntervalDomain>(&other);
    ASSERT(interval, "Comparing domains of different type");

    if (interval->isBottom()) return false;
    if (this->isBottom()) return true;
    if (this->isTop()) return false;

    return interval->lb_->le(lb_) && ub_->le(interval->ub_);
}

bool IntegerIntervalDomain::classof(const Domain *other) {
    return other->getType() == Type::INTEGER_INTERVAL;
}

size_t IntegerIntervalDomain::hashCode() const {
    return util::hash::simple_hash_value(value_, getType(), getWidth(), lb_, ub_);
}

std::string IntegerIntervalDomain::toPrettyString(const std::string&) const {
    if (isBottom()) return "[]";
    std::ostringstream ss;
    ss << "[" << lb_->toString() << ", " << ub_->toString() << "]";
    return ss.str();
}

///////////////////////////////////////////////////////////////
/// LLVM IR Semantics
///////////////////////////////////////////////////////////////

Domain::Ptr IntegerIntervalDomain::add(Domain::Ptr other) const {
    auto&& interval = llvm::dyn_cast<IntegerIntervalDomain>(other.get());
    ASSERT(interval, "Nullptr in interval");
    ASSERT(this->getWidth() == interval->getWidth(), "Adding two intervals of different format");

    if (this->isBottom() || interval->isBottom()) {
        return factory_->getInteger(getWidth());
    } else if (this->isTop() || interval->isTop()) {
        return factory_->getInteger(TOP, getWidth());
    } else {
        Integer::Ptr temp;
        auto lb = (temp = lb_->add(interval->lb_)) ? temp : Integer::getMinValue(getWidth());
        auto ub = (temp = ub_->add(interval->ub_)) ? temp : Integer::getMaxValue(getWidth());
        return factory_->getInteger(lb, ub);
    }
}

Domain::Ptr IntegerIntervalDomain::sub(Domain::Ptr other) const {
    auto&& interval = llvm::dyn_cast<IntegerIntervalDomain>(other.get());
    ASSERT(interval, "Nullptr in interval");
    ASSERT(this->getWidth() == interval->getWidth(), "Subtracting two intervals of different format");

    if (this->isBottom() || interval->isBottom()) {
        return factory_->getInteger(getWidth());
    } else if (this->isTop() || interval->isTop()) {
        return factory_->getInteger(TOP, getWidth());
    } else {
        Integer::Ptr temp;
        auto lb = (temp = lb_->sub(interval->ub_)) ? temp : Integer::getMinValue(getWidth());
        auto ub = (temp = ub_->sub(interval->lb_)) ? temp : Integer::getMaxValue(getWidth());
        return factory_->getInteger(lb, ub);
    }
}

Domain::Ptr IntegerIntervalDomain::mul(Domain::Ptr other) const {
    auto&& interval = llvm::dyn_cast<IntegerIntervalDomain>(other.get());
    ASSERT(interval, "Nullptr in interval");
    ASSERT(this->getWidth() == interval->getWidth(), "Multiplying two intervals of different format");

    if (this->isBottom() || interval->isBottom()) {
        return factory_->getInteger(getWidth());
    } else if (this->isTop() || interval->isTop()) {
        return factory_->getInteger(TOP, getWidth());
    } else {
        Integer::Ptr temp;
        auto ll = (temp = lb_->mul(interval->lb_)) ? temp : Integer::getMinValue(getWidth());
        auto ul = (temp = ub_->mul(interval->lb_)) ? temp : Integer::getMaxValue(getWidth());
        auto lu = (temp = lb_->mul(interval->ub_)) ? temp : Integer::getMinValue(getWidth());
        auto uu = (temp = ub_->mul(interval->ub_)) ? temp : Integer::getMaxValue(getWidth());

        using namespace util;
        auto lb = min(ll, ul, ll, ul);
        auto ub = max(ll, ul, ll, ul);

        return factory_->getInteger(lb, ub);
    }
}

#define DIV_OPERATION(OPER) \
    auto&& interval = llvm::dyn_cast<IntegerIntervalDomain>(other.get()); \
    ASSERT(interval, "Nullptr in interval"); \
    ASSERT(this->getWidth() == interval->getWidth(), "Dividing two intervals of different format"); \
    if (isBottom() || interval->isBottom()) { \
        return factory_->getInteger(getWidth()); \
    } else if (this->isTop() || interval->isTop()) { \
        return factory_->getInteger(TOP, getWidth()); \
    } else { \
        if (interval->isConstant(0)) { \
            return factory_->getInteger(TOP, getWidth()); \
        } else { \
            auto ll = lb_->OPER(interval->lb_); \
            auto ul = ub_->OPER(interval->lb_); \
            auto lu = lb_->OPER(interval->ub_); \
            auto uu = ub_->OPER(interval->ub_); \
            using namespace util; \
            auto lb = min(ll, ul, lu, uu); \
            auto ub = max(ll, ul, lu, uu); \
            return factory_->getInteger(lb, ub); \
        } \
    }


Domain::Ptr IntegerIntervalDomain::udiv(Domain::Ptr other) const {
    DIV_OPERATION(udiv);
}

Domain::Ptr IntegerIntervalDomain::sdiv(Domain::Ptr other) const {
    DIV_OPERATION(sdiv);
}

Domain::Ptr IntegerIntervalDomain::urem(Domain::Ptr other) const {
    DIV_OPERATION(urem);
}

Domain::Ptr IntegerIntervalDomain::srem(Domain::Ptr other) const {
    DIV_OPERATION(srem);
}

#define BIT_OPERATION(OPER) \
    auto&& interval = llvm::dyn_cast<IntegerIntervalDomain>(other.get()); \
    ASSERT(interval, "Nullptr in shl"); \
     \
    if (isBottom() || interval->isBottom()) { \
        return factory_->getInteger(getWidth()); \
    } else if (this->isTop() || interval->isTop()) { \
        return factory_->getInteger(TOP, getWidth()); \
    } else { \
        auto ll = lb_->OPER(interval->lb_); \
        auto ul = ub_->OPER(interval->lb_); \
        auto lu = lb_->OPER(interval->ub_); \
        auto uu = ub_->OPER(interval->ub_); \
        using namespace util; \
        auto lb = min(ll, ul, lu, uu); \
        auto ub = max(ll, ul, lu, uu); \
        return factory_->getInteger(lb, ub); \
    }

Domain::Ptr IntegerIntervalDomain::shl(Domain::Ptr other) const {
    BIT_OPERATION(shl);
}

Domain::Ptr IntegerIntervalDomain::lshr(Domain::Ptr other) const {
    BIT_OPERATION(lshr);
}

Domain::Ptr IntegerIntervalDomain::ashr(Domain::Ptr other) const {
    BIT_OPERATION(ashr);
}

Domain::Ptr IntegerIntervalDomain::bAnd(Domain::Ptr other) const {
    if (this->isBottom() || other->isBottom())
        return factory_->getInteger(getWidth());

    return factory_->getInteger(TOP, getWidth());
}

Domain::Ptr IntegerIntervalDomain::bOr(Domain::Ptr other) const {
    if (this->isBottom() || other->isBottom())
        return factory_->getInteger(getWidth());

    return factory_->getInteger(TOP, getWidth());
}

Domain::Ptr IntegerIntervalDomain::bXor(Domain::Ptr other) const {
    if (this->isBottom() || other->isBottom())
        return factory_->getInteger(getWidth());

    return factory_->getInteger(TOP, getWidth());
}

Domain::Ptr IntegerIntervalDomain::trunc(const llvm::Type& type) const {
    ASSERT(type.isIntegerTy(), "Non-integer type in trunc");
    auto&& intType = llvm::cast<llvm::IntegerType>(&type);

    if (isBottom()) return factory_->getInteger(BOTTOM, intType->getBitWidth());

    return factory_->getInteger(TOP, intType->getBitWidth());
}

Domain::Ptr IntegerIntervalDomain::zext(const llvm::Type& type) const {
    ASSERT(type.isIntegerTy(), "Non-integer type in zext");
    auto&& intType = llvm::cast<llvm::IntegerType>(&type);

    if (isBottom()) return factory_->getBottom(type);
    if (isTop()) return factory_->getTop(type);

    auto&& lb = lb_->zext(intType->getBitWidth());
    auto&& ub = ub_->zext(intType->getBitWidth());
    return factory_->getInteger(factory_->toInteger(0, intType->getBitWidth()),
                                util::max(lb, ub));
}

Domain::Ptr IntegerIntervalDomain::sext(const llvm::Type& type) const {
    ASSERT(type.isIntegerTy(), "Non-integer type in sext");
    auto&& intType = llvm::cast<llvm::IntegerType>(&type);

    if (isBottom()) return factory_->getBottom(type);
    if (isTop()) return factory_->getTop(type);

    auto&& lb = lb_->sext(intType->getBitWidth());
    auto&& ub = ub_->sext(intType->getBitWidth());
    return factory_->getInteger(lb, ub);
}

#define INT_TO_FP(OPER) \
    ASSERT(type.isFloatingPointTy(), "Non-FP type in inttofp"); \
    auto& newSemantics = util::getSemantics(type); \
    if (isBottom()) return factory_->getFloat(BOTTOM, newSemantics); \
    if (isTop()) return factory_->getFloat(TOP, newSemantics); \
     \
    unsigned width = 32; \
    if (type.isHalfTy()) \
        width = 16; \
    else if (type.isFloatTy()) \
        width = 32; \
    else if (type.isDoubleTy()) \
        width = 64; \
    else if (type.isFP128Ty()) \
        width = 128; \
    else if (type.isPPC_FP128Ty()) \
        width = 128; \
    else if (type.isX86_FP80Ty()) \
        width = 80; \
    Integer::Ptr newLB = lb_, newUB = ub_; \
    if (width < getWidth()) { \
        newLB = lb_->trunc(width); \
        newUB = ub_->trunc(width); \
    } else if (width > getWidth()) { \
        newLB = lb_->OPER(width); \
        newUB = ub_->OPER(width); \
    } \
    llvm::APFloat lb = util::getMinValue(newSemantics), ub = util::getMinValue(newSemantics); \
    if (newLB->isMin()) lb = util::getMinValue(newSemantics); \
    else if (newLB->isMax()) lb = util::getMaxValue(newSemantics); \
    else lb = llvm::APFloat(newSemantics, newLB->toString()); \
    if (newUB->isMin()) ub = util::getMinValue(newSemantics); \
    else if (newUB->isMax()) ub = util::getMaxValue(newSemantics); \
    else ub = llvm::APFloat(newSemantics, newUB->toString()); \
    return factory_->getFloat(lb, ub);

Domain::Ptr IntegerIntervalDomain::uitofp(const llvm::Type& type) const {
    INT_TO_FP(zext)
}

Domain::Ptr IntegerIntervalDomain::sitofp(const llvm::Type& type) const {
    INT_TO_FP(sext)
}

Domain::Ptr IntegerIntervalDomain::inttoptr(const llvm::Type& type) const {
    return factory_->getTop(type);
}

Domain::Ptr IntegerIntervalDomain::bitcast(const llvm::Type& type) {
    return factory_->getTop(type);
}

Domain::Ptr IntegerIntervalDomain::icmp(Domain::Ptr other, llvm::CmpInst::Predicate operation) const {
    auto&& getBool = [&] (bool val) -> Domain::Ptr {
        llvm::APInt retval(1, 0, false);
        if (val) retval = 1;
        else retval = 0;
        return factory_->getInteger(factory_->toInteger(retval));
    };

    if (this->isBottom() || other->isBottom()) {
        return factory_->getInteger(1);
    } else if (this->isTop() || other->isTop()) {
        return factory_->getInteger(TOP, 1);
    }

    auto&& interval = llvm::dyn_cast<IntegerIntervalDomain>(other.get());
    ASSERT(interval, "Nullptr in interval");
    ASSERT(this->getWidth() == interval->getWidth(), "Joining two intervals of different format");

    switch (operation) {
        case llvm::CmpInst::ICMP_EQ:
            if (this->isConstant() && interval->isConstant() && lb_->eq(interval->lb_)) {
                return getBool(true);
            } else if (this->hasIntersection(interval)) {
                return factory_->getInteger(TOP, 1);
            } else {
                return getBool(false);
            }

        case llvm::CmpInst::ICMP_NE:
            if (this->isConstant() && interval->isConstant() && lb_->eq(interval->lb_)) {
                return getBool(false);
            } else if (this->hasIntersection(interval)) {
                return factory_->getInteger(TOP, 1);
            } else {
                return getBool(true);
            }

        case llvm::CmpInst::ICMP_SGE:
            // if we don't know exact bounds of interval, we can't say anything abound signed comparsion
            if (not (lb_->isValue() || ub_->isValue() || interval->lb_->isValue() || interval->ub_->isValue()))
                return factory_->getInteger(TOP, 1);

            if (signed_lb()->sge(interval->signed_ub())) {
                return getBool(true);
            } else if (signed_ub()->slt(interval->signed_lb())) {
                return getBool(false);
            } else {
                return factory_->getInteger(TOP, 1);
            }

        case llvm::CmpInst::ICMP_SGT:
            // if we don't know exact bounds of interval, we can't say anything abound signed comparsion
            if (not (lb_->isValue() || ub_->isValue() || interval->lb_->isValue() || interval->ub_->isValue()))
                return factory_->getInteger(TOP, 1);

            if (signed_lb()->sgt(interval->signed_ub())) {
                return getBool(true);
            } else if (signed_ub()->sle(interval->signed_lb())) {
                return getBool(false);
            } else {
                return factory_->getInteger(TOP, 1);
            }

        case llvm::CmpInst::ICMP_SLE:
            // if we don't know exact bounds of interval, we can't say anything abound signed comparsion
            if (not (lb_->isValue() || ub_->isValue() || interval->lb_->isValue() || interval->ub_->isValue()))
                return factory_->getInteger(TOP, 1);

            if (signed_ub()->sle(interval->signed_lb())) {
                return getBool(true);
            } else if (signed_lb()->sgt(interval->signed_ub())) {
                return getBool(false);
            } else {
                return factory_->getInteger(TOP, 1);
            }

        case llvm::CmpInst::ICMP_SLT:
            // if we don't know exact bounds of interval, we can't say anything abound signed comparsion
            if (not (lb_->isValue() || ub_->isValue() || interval->lb_->isValue() || interval->ub_->isValue()))
                return factory_->getInteger(TOP, 1);

            if (signed_ub()->slt(interval->signed_lb())) {
                return getBool(true);
            } else if (signed_lb()->sge(interval->signed_ub())) {
                return getBool(false);
            } else {
                return factory_->getInteger(TOP, 1);
            }

        case llvm::CmpInst::ICMP_UGE:
            if (lb_->ge(interval->ub_)) {
                return getBool(true);
            } else if (ub_->lt(interval->lb_)) {
                return getBool(false);
            } else {
                return factory_->getInteger(TOP, 1);
            }

        case llvm::CmpInst::ICMP_UGT:
            if (lb_->gt(interval->ub_)) {
                return getBool(true);
            } else if (ub_->le(interval->lb_)) {
                return getBool(false);
            } else {
                return factory_->getInteger(TOP, 1);
            }

        case llvm::CmpInst::ICMP_ULE:
            if (ub_->le(interval->lb_)) {
                return getBool(true);
            } else if (lb_->gt(interval->ub_)) {
                return getBool(false);
            } else {
                return factory_->getInteger(TOP, 1);
            }

        case llvm::CmpInst::ICMP_ULT:
            if (ub_->lt(interval->lb_)) {
                return getBool(true);
            } else if (lb_->ge(interval->ub_)) {
                return getBool(false);
            } else {
                return factory_->getInteger(TOP, 1);
            }

        default:
            UNREACHABLE("Unknown operation in icmp");
    }
}

Split IntegerIntervalDomain::splitByEq(Domain::Ptr other) {
    auto interval = llvm::dyn_cast<IntegerIntervalDomain>(other.get());
    ASSERT(interval, "Not interval in split");

    return interval->isConstant() ?
           Split{ interval->shared_from_this(), shared_from_this() } :
           Split{ shared_from_this(), shared_from_this() };
}

Split IntegerIntervalDomain::splitByLess(Domain::Ptr other) {
    auto interval = llvm::dyn_cast<IntegerIntervalDomain>(other.get());
    ASSERT(interval, "Not interval in split");

    if (this->operator<(*other.get())) return {shared_from_this(), shared_from_this()};

    auto trueVal = factory_->getInteger(lb_, interval->ub_);
    auto falseVal = factory_->getInteger(interval->lb_, this->ub_);
    return {trueVal, falseVal};
}

Split IntegerIntervalDomain::splitBySLess(Domain::Ptr other) {
    auto interval = llvm::dyn_cast<IntegerIntervalDomain>(other.get());
    ASSERT(interval, "Not interval in split");

    if (this->operator<(*other.get())) return {shared_from_this(), shared_from_this()};

    auto trueVal = factory_->getInteger(util::min(this->signed_lb(), interval->signed_ub()),
                                        util::max(this->signed_lb(), interval->signed_ub()));
    auto falseVal = factory_->getInteger(util::min(interval->signed_lb(), this->signed_ub()),
                                         util::max(interval->signed_lb(), this->signed_ub()));
    return {trueVal, falseVal};
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"