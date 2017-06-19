//
// Created by abdullin on 2/2/17.
//

#include "DomainFactory.h"
#include "IntegerInterval.h"
#include "Util/sayonara.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

IntegerInterval::IntegerInterval(DomainFactory* factory, Integer::Ptr constant) :
        IntegerInterval(factory, constant, constant) {}

IntegerInterval::IntegerInterval(Domain::Value value, DomainFactory* factory, unsigned width) :
        IntegerInterval(factory, std::make_tuple(value,
                                                 factory->toInteger(0, width),
                                                 factory->toInteger(0, width))) {}

IntegerInterval::IntegerInterval(DomainFactory* factory, Integer::Ptr from, Integer::Ptr to) :
        IntegerInterval(factory, std::make_tuple(VALUE,
                                                 from,
                                                 to)) {}

IntegerInterval::IntegerInterval(DomainFactory* factory, const IntegerInterval::ID& key) :
        Domain(std::get<0>(key), Type::INTEGER_INTERVAL, factory),
        from_(std::get<1>(key)),
        to_(std::get<2>(key)) {
    ASSERT(from_->getWidth() == to_->getWidth(), "Different bit width of interval bounds");
    ASSERT(from_->le(to_), "Lower bound is greater that upper bound");
    if (from_->isMin() && to_->isMax()) value_ = TOP;
    else if (value_ == TOP) setTop();
}

void IntegerInterval::setTop() {
    Domain::setTop();
    from_ = util::getMinValue(getWidth());
    to_ = util::getMaxValue(getWidth());
}

Domain::Ptr IntegerInterval::join(Domain::Ptr other) const {
    auto&& interval = llvm::dyn_cast<IntegerInterval>(other.get());
    ASSERT(interval, "Nullptr in interval join");
    ASSERT(this->getWidth() == interval->getWidth(), "Joining two intervals of different format");

    if (interval->isBottom()) {
        return shared_from_this();
    } else if (this->isBottom()) {
        return interval->shared_from_this();
    } else {
        auto left = util::min(from_, interval->from_);
        auto right = util::max(to_, interval->to_);
        return factory_->getInteger(left, right);
    }
}

Domain::Ptr IntegerInterval::meet(Domain::Ptr other) const {
    auto&& interval = llvm::dyn_cast<IntegerInterval>(other.get());
    ASSERT(interval, "Nullptr in interval meet");
    ASSERT(this->getWidth() == interval->getWidth(), "Joining two intervals of different format");

    if (this->isBottom()) {
        return shared_from_this();
    } else if (interval->isBottom()) {
        return interval->shared_from_this();
    } else {
        auto left = from_->lt(interval->from_) ? interval->from_ : from_;
        auto right = to_->lt(interval->to_) ? to_ : interval->to_;

        return left->gt(right) ?
               shared_from_this() :
               factory_->getInteger(left, right);
    }
}

Domain::Ptr IntegerInterval::widen(Domain::Ptr other) const {
    auto&& interval = llvm::dyn_cast<IntegerInterval>(other.get());
    ASSERT(interval, "Nullptr in interval");
    ASSERT(this->getWidth() == interval->getWidth(), "Widening two intervals of different format");

    if (interval->isBottom()) {
        return shared_from_this();
    } else if (this->isBottom()) {
        return interval->shared_from_this();
    }

    auto left = interval->from_->lt(from_) ? util::getMinValue(getWidth()) : from_;

    Integer::Ptr nextRight;
    auto ten = factory_->toInteger(10, getWidth());
    if (to_->getValue() == 0) {
        nextRight = ten;
    } else {
        auto temp = to_->mul(ten);
        nextRight = temp ? temp : util::getMaxValue(getWidth());
    }
    auto right = to_->lt(interval->to_) ? nextRight : to_;

    return factory_->getInteger(left, right);
}

Domain::Ptr IntegerInterval::narrow(Domain::Ptr other) const {
    auto&& interval = llvm::dyn_cast<IntegerInterval>(other.get());
    ASSERT(interval, "Nullptr in interval");
    ASSERT(this->getWidth() == interval->getWidth(), "Widening two intervals of different format");

    if (this->isBottom()) {
        return shared_from_this();
    } else if (interval->isBottom()) {
        return interval->shared_from_this();
    }

    auto left = from_->isMin() ? interval->from_ : from_;
    auto right = to_->isMax() ? interval->to_ : to_;

    return factory_->getInteger(left, right);
}

size_t IntegerInterval::getWidth() const { return from_->getWidth(); }
bool IntegerInterval::isConstant() const { return from_->eq(to_); }
bool IntegerInterval::isConstant(uint64_t constant) const {
    auto constInteger = factory_->toInteger(constant, getWidth());
    return isConstant() && from_->eq(constInteger);
}
Integer::Ptr IntegerInterval::from() const { return from_; }
Integer::Ptr IntegerInterval::to() const { return to_; }
Integer::Ptr IntegerInterval::signedFrom() const { return util::min(from_, to_, true); }
Integer::Ptr IntegerInterval::signedTo() const { return util::max(from_, to_, true); }

/// Assumes that both intervals have value
bool IntegerInterval::hasIntersection(const IntegerInterval* other) const {
    ASSERT(this->isValue() && other->isValue(), "Not value intervals");

    if (from_->le(other->from_) && other->from_->le(to_)) {
        return true;
    } else if (other->from_->le(to_) &&  to_->le(other->to_)) {
        return true;
    }
    return false;
}

bool IntegerInterval::hasIntersection(Integer::Ptr constant) const {
    return from_->le(constant) && constant->le(to_);
}

bool IntegerInterval::equals(const Domain *other) const {
    auto&& interval = llvm::dyn_cast<IntegerInterval>(other);
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
            from_->eq(interval->from_) &&
            to_->eq(interval->to_);
}

bool IntegerInterval::operator<(const Domain &other) const {
    auto&& interval = llvm::dyn_cast<IntegerInterval>(&other);
    ASSERT(interval, "Comparing domains of different type");

    if (interval->isBottom()) return false;
    if (this->isBottom()) return true;
    if (this->isTop()) return false;

    return interval->from_->le(from_) && to_->le(interval->to_);
}

bool IntegerInterval::classof(const Domain *other) {
    return other->getType() == Type::INTEGER_INTERVAL;
}

size_t IntegerInterval::hashCode() const {
    return util::hash::simple_hash_value(value_, getType(), getWidth(),
                                         from_, to_);
}

std::string IntegerInterval::toPrettyString(const std::string&) const {
    if (isBottom()) return "[]";
    std::ostringstream ss;
    ss << "[" << from_->toString() << ", " << to_->toString() << "]";
    return ss.str();
}

///////////////////////////////////////////////////////////////
/// LLVM IR Semantics
///////////////////////////////////////////////////////////////

Domain::Ptr IntegerInterval::add(Domain::Ptr other) const {
    auto&& interval = llvm::dyn_cast<IntegerInterval>(other.get());
    ASSERT(interval, "Nullptr in interval");
    ASSERT(this->getWidth() == interval->getWidth(), "Adding two intervals of different format");

    if (this->isBottom() || interval->isBottom()) {
        return factory_->getInteger(getWidth());
    } else if (this->isTop() || interval->isTop()) {
        return factory_->getInteger(TOP, getWidth());
    } else {
        Integer::Ptr temp;
        auto left = (temp = from_->add(interval->from_)) ? temp : util::getMinValue(getWidth());
        auto right = (temp = to_->add(interval->to_)) ? temp : util::getMaxValue(getWidth());
        return factory_->getInteger(left, right);
    }
}

Domain::Ptr IntegerInterval::sub(Domain::Ptr other) const {
    auto&& interval = llvm::dyn_cast<IntegerInterval>(other.get());
    ASSERT(interval, "Nullptr in interval");
    ASSERT(this->getWidth() == interval->getWidth(), "Subtracting two intervals of different format");

    if (this->isBottom() || interval->isBottom()) {
        return factory_->getInteger(getWidth());
    } else if (this->isTop() || interval->isTop()) {
        return factory_->getInteger(TOP, getWidth());
    } else {
        Integer::Ptr temp;
        auto left = (temp = from_->sub(interval->to_)) ? temp : util::getMinValue(getWidth());
        auto right = (temp = to_->sub(interval->from_)) ? temp : util::getMaxValue(getWidth());
        return factory_->getInteger(left, right);
    }
}

Domain::Ptr IntegerInterval::mul(Domain::Ptr other) const {
    auto&& interval = llvm::dyn_cast<IntegerInterval>(other.get());
    ASSERT(interval, "Nullptr in interval");
    ASSERT(this->getWidth() == interval->getWidth(), "Multiplying two intervals of different format");

    if (this->isBottom() || interval->isBottom()) {
        return factory_->getInteger(getWidth());
    } else if (this->isTop() || interval->isTop()) {
        return factory_->getInteger(TOP, getWidth());
    } else {
        Integer::Ptr temp;
        auto fromFrom = (temp = from_->mul(interval->from_)) ? temp : util::getMinValue(getWidth());
        auto toFrom = (temp = to_->mul(interval->from_)) ? temp : util::getMaxValue(getWidth());
        auto fromTo = (temp = from_->mul(interval->to_)) ? temp : util::getMinValue(getWidth());
        auto toTo = (temp = to_->mul(interval->to_)) ? temp : util::getMaxValue(getWidth());

        using namespace util;
        auto first = factory_->getInteger(min(fromFrom, toFrom), max(fromFrom, toFrom));
        auto second = factory_->getInteger(min(fromTo, toTo), max(fromTo, toTo));

        return first->join(second);
    }
}

#define DIV_OPERATION(OPER) \
    auto&& interval = llvm::dyn_cast<IntegerInterval>(other.get()); \
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
            using namespace util; \
            auto first = factory_->getInteger(min(from_->OPER(interval->from_), to_->OPER(interval->from_)), \
                                      max(from_->OPER(interval->from_), to_->OPER(interval->from_))); \
            auto second = factory_->getInteger(min(from_->OPER(interval->to_), to_->OPER(interval->to_)), \
                                       max(from_->OPER(interval->to_), to_->OPER(interval->to_))); \
            return first->join(second); \
        } \
    }


Domain::Ptr IntegerInterval::udiv(Domain::Ptr other) const {
    DIV_OPERATION(udiv);
}

Domain::Ptr IntegerInterval::sdiv(Domain::Ptr other) const {
    DIV_OPERATION(sdiv);
}

Domain::Ptr IntegerInterval::urem(Domain::Ptr other) const {
    DIV_OPERATION(urem);
}

Domain::Ptr IntegerInterval::srem(Domain::Ptr other) const {
    DIV_OPERATION(srem);
}

#define BIT_OPERATION(OPER) \
    auto&& interval = llvm::dyn_cast<IntegerInterval>(other.get()); \
    ASSERT(interval, "Nullptr in shl"); \
     \
    if (isBottom() || interval->isBottom()) { \
        return factory_->getInteger(getWidth()); \
    } else if (this->isTop() || interval->isTop()) { \
        return factory_->getInteger(TOP, getWidth()); \
    } else { \
        using namespace util; \
        Domain::Ptr first = factory_->getInteger(min(from_->OPER(interval->from_), to_->OPER(interval->from_)), \
                                                 max(from_->OPER(interval->from_), to_->OPER(interval->from_))); \
        Domain::Ptr second = factory_->getInteger(min(from_->OPER(interval->to_), to_->OPER(interval->to_)), \
                                                  max(from_->OPER(interval->to_), to_->OPER(interval->to_))); \
         \
        return first->join(second); \
    }

Domain::Ptr IntegerInterval::shl(Domain::Ptr other) const {
    BIT_OPERATION(shl);
}

Domain::Ptr IntegerInterval::lshr(Domain::Ptr other) const {
    BIT_OPERATION(lshr);
}

Domain::Ptr IntegerInterval::ashr(Domain::Ptr other) const {
    BIT_OPERATION(ashr);
}

Domain::Ptr IntegerInterval::bAnd(Domain::Ptr other) const {
    if (this->isBottom() || other->isBottom())
        return factory_->getInteger(getWidth());

    return factory_->getInteger(TOP, getWidth());
}

Domain::Ptr IntegerInterval::bOr(Domain::Ptr other) const {
    if (this->isBottom() || other->isBottom())
        return factory_->getInteger(getWidth());

    return factory_->getInteger(TOP, getWidth());
}

Domain::Ptr IntegerInterval::bXor(Domain::Ptr other) const {
    if (this->isBottom() || other->isBottom())
        return factory_->getInteger(getWidth());

    return factory_->getInteger(TOP, getWidth());
}

Domain::Ptr IntegerInterval::trunc(const llvm::Type& type) const {
    ASSERT(type.isIntegerTy(), "Non-integer type in trunc");
    auto&& intType = llvm::cast<llvm::IntegerType>(&type);

    if (isBottom()) return factory_->getInteger(BOTTOM, intType->getBitWidth());

    return factory_->getInteger(TOP, intType->getBitWidth());
}

Domain::Ptr IntegerInterval::zext(const llvm::Type& type) const {
    ASSERT(type.isIntegerTy(), "Non-integer type in zext");
    auto&& intType = llvm::cast<llvm::IntegerType>(&type);

    if (isBottom()) return factory_->getBottom(type);
    if (isTop()) return factory_->getTop(type);

    auto&& newFrom = from_->zext(intType->getBitWidth());
    auto&& newTo = to_->zext(intType->getBitWidth());
    return factory_->getInteger(factory_->toInteger(llvm::APInt(intType->getBitWidth(), 0)),
                                util::max(newFrom, newTo));
}

Domain::Ptr IntegerInterval::sext(const llvm::Type& type) const {
    ASSERT(type.isIntegerTy(), "Non-integer type in sext");
    auto&& intType = llvm::cast<llvm::IntegerType>(&type);

    if (isBottom()) return factory_->getBottom(type);
    if (isTop()) return factory_->getTop(type);

    auto&& newFrom = from_->sext(intType->getBitWidth());
    auto&& newTo = to_->sext(intType->getBitWidth());
    return factory_->getInteger(newFrom, newTo);
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
    Integer::Ptr newFrom = from_, newTo = to_; \
    if (width <  getWidth()) { \
        newFrom = from_->trunc(width); \
        newTo = to_->trunc(width); \
    } else if (width >  getWidth()) { \
        newFrom = from_->OPER(width); \
        newTo = to_->OPER(width); \
    } \
    llvm::APFloat from = util::getMinValue(newSemantics), to = util::getMinValue(newSemantics); \
    if (newFrom->isMin()) from = util::getMinValue(newSemantics); \
    else if (newFrom->isMax()) from = util::getMaxValue(newSemantics); \
    else from = llvm::APFloat(newSemantics, newFrom->toString()); \
    if (newTo->isMin()) to = util::getMinValue(newSemantics); \
    else if (newTo->isMax()) to = util::getMaxValue(newSemantics); \
    else to = llvm::APFloat(newSemantics, newTo->toString()); \
    return factory_->getFloat(from, to);

Domain::Ptr IntegerInterval::uitofp(const llvm::Type& type) const {
    INT_TO_FP(zext)
}

Domain::Ptr IntegerInterval::sitofp(const llvm::Type& type) const {
    INT_TO_FP(sext)
}

Domain::Ptr IntegerInterval::inttoptr(const llvm::Type& type) const {
    return factory_->getTop(type);
}

Domain::Ptr IntegerInterval::bitcast(const llvm::Type& type) const {
    return factory_->getTop(type);
}

Domain::Ptr IntegerInterval::icmp(Domain::Ptr other, llvm::CmpInst::Predicate operation) const {
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

    auto&& interval = llvm::dyn_cast<IntegerInterval>(other.get());
    ASSERT(interval, "Nullptr in interval");
    ASSERT(this->getWidth() == interval->getWidth(), "Joining two intervals of different format");

    switch (operation) {
        case llvm::CmpInst::ICMP_EQ:
            if (this->isConstant() && interval->isConstant() && from()->eq(interval->from())) {
                return getBool(true);
            } else if (this->hasIntersection(interval)) {
                return factory_->getInteger(TOP, 1);
            } else {
                return getBool(false);
            }

        case llvm::CmpInst::ICMP_NE:
            if (this->isConstant() && interval->isConstant() && from()->eq(interval->from())) {
                return getBool(false);
            } else if (this->hasIntersection(interval)) {
                return factory_->getInteger(TOP, 1);
            } else {
                return getBool(true);
            }

        case llvm::CmpInst::ICMP_SGE:
            if (signedFrom()->sge(interval->signedTo())) {
                return getBool(true);
            } else if (signedTo()->slt(interval->signedFrom())) {
                return getBool(false);
            } else {
                return factory_->getInteger(TOP, 1);
            }

        case llvm::CmpInst::ICMP_SGT:
            if (signedFrom()->sgt(interval->signedTo())) {
                return getBool(true);
            } else if (signedTo()->sle(interval->signedFrom())) {
                return getBool(false);
            } else {
                return factory_->getInteger(TOP, 1);
            }

        case llvm::CmpInst::ICMP_SLE:
            if (signedTo()->sle(interval->signedFrom())) {
                return getBool(true);
            } else if (signedFrom()->sgt(interval->signedTo())) {
                return getBool(false);
            } else {
                return factory_->getInteger(TOP, 1);
            }

        case llvm::CmpInst::ICMP_SLT:
            if (signedTo()->slt(interval->signedFrom())) {
                return getBool(true);
            } else if (signedFrom()->sge(interval->signedTo())) {
                return getBool(false);
            } else {
                return factory_->getInteger(TOP, 1);
            }

        case llvm::CmpInst::ICMP_UGE:
            if (from()->ge(interval->to())) {
                return getBool(true);
            } else if (to()->lt(interval->from())) {
                return getBool(false);
            } else {
                return factory_->getInteger(TOP, 1);
            }

        case llvm::CmpInst::ICMP_UGT:
            if (from()->gt(interval->to())) {
                return getBool(true);
            } else if (to()->le(interval->from())) {
                return getBool(false);
            } else {
                return factory_->getInteger(TOP, 1);
            }

        case llvm::CmpInst::ICMP_ULE:
            if (to()->le(interval->from())) {
                return getBool(true);
            } else if (from()->gt(interval->to())) {
                return getBool(false);
            } else {
                return factory_->getInteger(TOP, 1);
            }

        case llvm::CmpInst::ICMP_ULT:
            if (to()->lt(interval->from())) {
                return getBool(true);
            } else if (from()->ge(interval->to())) {
                return getBool(false);
            } else {
                return factory_->getInteger(TOP, 1);
            }

        default:
            UNREACHABLE("Unknown operation in icmp");
    }
}

Split IntegerInterval::splitByEq(Domain::Ptr other) const {
    auto interval = llvm::dyn_cast<IntegerInterval>(other.get());
    ASSERT(interval, "Not interval in split");

    return interval->isConstant() ?
           Split{ interval->shared_from_this(), shared_from_this() } :
           Split{ shared_from_this(), shared_from_this() };
}

Split IntegerInterval::splitByNeq(Domain::Ptr other) const {
    auto interval = llvm::dyn_cast<IntegerInterval>(other.get());
    ASSERT(interval, "Not interval in split");

    if (this->operator<(*other.get())) return {shared_from_this(), shared_from_this()};

    return interval->isConstant() ?
           Split{ shared_from_this(), interval->shared_from_this() } :
           Split{ shared_from_this(), shared_from_this() };
}

Split IntegerInterval::splitByLess(Domain::Ptr other) const {
    auto interval = llvm::dyn_cast<IntegerInterval>(other.get());
    ASSERT(interval, "Not interval in split");

    if (this->operator<(*other.get())) return {shared_from_this(), shared_from_this()};

    auto trueVal = factory_->getInteger(from_, interval->to_);
    auto falseVal = factory_->getInteger(interval->from_, this->to_);
    return {trueVal, falseVal};
}

Split IntegerInterval::splitBySLess(Domain::Ptr other) const {
    auto interval = llvm::dyn_cast<IntegerInterval>(other.get());
    ASSERT(interval, "Not interval in split");

    if (this->operator<(*other.get())) return {shared_from_this(), shared_from_this()};

    auto trueVal = factory_->getInteger(util::min(this->signedFrom(), interval->signedTo()),
                                        util::max(this->signedFrom(), interval->signedTo()));
    auto falseVal = factory_->getInteger(util::min(interval->signedFrom(), this->signedTo()),
                                         util::max(interval->signedFrom(), this->signedTo()));
    return {trueVal, falseVal};
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"