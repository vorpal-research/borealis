//
// Created by abdullin on 2/2/17.
//

#include <llvm/Support/raw_ostream.h>
#include "DomainFactory.h"
#include "IntegerInterval.h"
#include "Util/sayonara.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

IntegerInterval::IntegerInterval(DomainFactory* factory, unsigned width, bool isSigned) :
        IntegerInterval(BOTTOM, factory, width, isSigned) {}

IntegerInterval::IntegerInterval(DomainFactory* factory, const llvm::APInt& constant, bool isSigned) :
        IntegerInterval(factory, constant, constant, isSigned) {}

IntegerInterval::IntegerInterval(Domain::Value value, DomainFactory* factory, unsigned width, bool isSigned) :
        IntegerInterval(factory, std::make_tuple(value,
                                                 isSigned,
                                                 llvm::APInt(width, 0, false),
                                                 llvm::APInt(width, 0, false))) {}

IntegerInterval::IntegerInterval(DomainFactory* factory, const llvm::APInt& from, const llvm::APInt& to, bool isSigned) :
        IntegerInterval(factory, std::make_tuple(VALUE,
                                                 isSigned,
                                                 from,
                                                 to)) {}

IntegerInterval::IntegerInterval(const IntegerInterval &interval) :
        Domain(interval.value_, Type::IntegerInterval, interval.factory_),
        signed_(interval.signed_),
        from_(interval.from_),
        to_(interval.to_) {}

IntegerInterval::IntegerInterval(DomainFactory* factory, const IntegerInterval::ID& key) :
        Domain(std::get<0>(key), Type::IntegerInterval, factory),
        signed_(std::get<1>(key)),
        from_(std::get<2>(key)),
        to_(std::get<3>(key)) {
    ASSERT(from_.getBitWidth() == to_.getBitWidth(), "Different bit width of interval bounds");
    ASSERT(isCorrect(), "Interval lower bound is greater than upper bound:" + from_.toString(10, false) + " " + to_.toString(10, false));
    if (util::isMinValue(from_, signed_) && util::isMaxValue(to_, signed_)) value_ = TOP;
    else if (value_ == TOP) setTop();
}

void IntegerInterval::setTop() {
    Domain::setTop();
    from_ = util::getMinValue(getWidth(), signed_);
    to_ = util::getMaxValue(getWidth(), signed_);
}

Domain::Ptr IntegerInterval::join(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<IntegerInterval>(other.get());
    ASSERT(value, "Nullptr in interval join");
    ASSERT(this->getWidth() == value->getWidth(), "Joining two intervals of different format");

    if (value->isBottom()) {
        return shared_from_this();
    } else if (this->isBottom()) {
        return value->shared_from_this();
    } else {
        llvm::APInt left = util::min(from_, value->from_, signed_);
        llvm::APInt right = util::max(to_, value->to_, signed_);
        return factory_->getInteger(left, right, signed_);
    }
}

Domain::Ptr IntegerInterval::meet(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<IntegerInterval>(other.get());
    ASSERT(value, "Nullptr in interval meet");
    ASSERT(this->getWidth() == value->getWidth(), "Joining two intervals of different format");

    if (this->isBottom()) {
        return shared_from_this();
    } else if (value->isBottom()) {
        return value->shared_from_this();
    } else {
        using borealis::util::lt;
        llvm::APInt left = lt(from_, value->from_, signed_) ? value->from_ : from_;
        llvm::APInt right = lt(to_, value->to_, signed_) ? to_ : value->to_;

        return util::gt(left, right, signed_) ?
               shared_from_this() :
               factory_->getInteger(left, right, signed_);
    }
}

Domain::Ptr IntegerInterval::widen(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<IntegerInterval>(other.get());
    ASSERT(value, "Nullptr in interval");
    ASSERT(this->getWidth() == value->getWidth(), "Widening two intervals of different format");

    if (value->isBottom()) {
        return shared_from_this();
    } else if (this->isBottom()) {
        return value->shared_from_this();
    }

    auto left = (util::lt(value->from_, from_, signed_)) ? util::getMinValue(getWidth(), signed_) : from_;
    auto right = (util::lt(to_, value->to_, signed_)) ? util::getMaxValue(getWidth(), signed_) : to_;

    return factory_->getInteger(left, right, signed_);
}

Domain::Ptr IntegerInterval::narrow(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<IntegerInterval>(other.get());
    ASSERT(value, "Nullptr in interval");
    ASSERT(this->getWidth() == value->getWidth(), "Widening two intervals of different format");

    if (this->isBottom()) {
        return shared_from_this();
    } else if (value->isBottom()) {
        return value->shared_from_this();
    }

    auto left = (util::eq(from_, util::getMinValue(getWidth(), signed_))) ? value->from_ : from_;
    auto right = (util::eq(to_, util::getMaxValue(getWidth(), signed_))) ? value->to_ : to_;

    return factory_->getInteger(left, right, signed_);
}

unsigned IntegerInterval::getWidth() const { return from_.getBitWidth(); }
bool IntegerInterval::isConstant() const { return util::eq(from_, to_); }
bool IntegerInterval::isConstant(uint64_t constant) const { return isConstant() && from_ == constant; }
const llvm::APInt& IntegerInterval::from() const { return from_; }
const llvm::APInt& IntegerInterval::to() const { return to_; }

/// Assumes that both intervals have value
bool IntegerInterval::intersects(const IntegerInterval* other) const {
    ASSERT(this->isValue() && other->isValue(), "Not value intervals");

    if (util::le(from_, other->from_, signed_) && util::le(other->from_, to_, signed_)) {
        return true;
    } else if ( util::le(other->from_, to_, signed_) &&  util::le(to_, other->to_, signed_)) {
        return true;
    }
    return false;
}

bool IntegerInterval::intersects(const llvm::APInt& constant) const {
    return util::le(from_, constant, signed_) && util::le(to_, constant, signed_);
}

bool IntegerInterval::isCorrect() {
    if (util::le(from_, to_, signed_)) return true;
//    if (util::le(from_, to_, not signed_)) {
//        signed_ = not signed_;
//        return true;
//    }
    return false;
}

bool IntegerInterval::equals(const Domain *other) const {
    auto&& value = llvm::dyn_cast<IntegerInterval>(other);
    if (not value) return false;
    if (this == other) return true;

    if (this->isBottom() && value->isBottom()) {
        return this->getWidth() == value->getWidth();
    }
    if (this->isTop() && value->isTop()) {
        return this->getWidth() == value->getWidth();
    }

    return  this->value_ == value->value_ &&
            this->getWidth() == value->getWidth() &&
            util::eq(from_, value->from_) &&
            util::eq(to_, value->to_);
}

bool IntegerInterval::operator<(const Domain &other) const {
    auto&& value = llvm::dyn_cast<IntegerInterval>(&other);
    ASSERT(value, "Comparing domains of different type");

    if (value->isBottom()) return false;
    if (this->isBottom()) return true;
    if (this->isTop()) return false;

    return (util::le(value->from_, from_, signed_) && util::le(to_, value->to_, signed_));
}

bool IntegerInterval::classof(const Domain *other) {
    return other->getType() == Type::IntegerInterval;
}

size_t IntegerInterval::hashCode() const {
    return util::hash::simple_hash_value(value_, getType(), getWidth(),
                                         from_, to_);
}

std::string IntegerInterval::toString() const {
    if (isBottom()) return "[]";
    std::ostringstream ss;
    ss << "[" << util::toString(from_, signed_) << ", " << util::toString(to_, signed_) << "]";
    return ss.str();
}

Domain *IntegerInterval::clone() const {
    return new IntegerInterval(*this);
}

///////////////////////////////////////////////////////////////
/// LLVM IR Semantics
///////////////////////////////////////////////////////////////

Domain::Ptr IntegerInterval::add(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<IntegerInterval>(other.get());
    ASSERT(value, "Nullptr in interval");
    ASSERT(this->getWidth() == value->getWidth(), "Adding two intervals of different format");

    if (this->isBottom() || value->isBottom()) {
        return factory_->getInteger(getWidth(), signed_);
    } else if (this->isTop() || value->isTop(), signed_) {
        return factory_->getInteger(TOP, getWidth());
    } else {
        bool ovLeft = false, ovRight = false;
        llvm::APInt left, right;
        if (signed_) {
            left = from_.sadd_ov(value->from_, ovLeft);
            right = to_.sadd_ov(value->to_, ovRight);
        } else {
            left = from_.uadd_ov(value->from_, ovLeft);
            right = to_.uadd_ov(value->to_, ovRight);
        }
        if (ovLeft) left = util::getMinValue( getWidth(), signed_);
        if (ovRight) right = util::getMaxValue( getWidth(), signed_);
        return factory_->getInteger(left, right, signed_);
    }
}

Domain::Ptr IntegerInterval::sub(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<IntegerInterval>(other.get());
    ASSERT(value, "Nullptr in interval");
    ASSERT(this->getWidth() == value->getWidth(), "Subtracting two intervals of different format");

    if (this->isBottom() || value->isBottom()) {
        return factory_->getInteger(getWidth(), signed_);
    } else if (this->isTop() || value->isTop()) {
        return factory_->getInteger(TOP, getWidth(), signed_);
    } else {
        bool ovLeft = false, ovRight = false;
        llvm::APInt left, right;
        if (signed_) {
            left = from_.ssub_ov(value->to_, ovLeft);
            right = to_.ssub_ov(value->from_, ovRight);
        } else {
            left = from_.usub_ov(value->to_, ovLeft);
            right = to_.usub_ov(value->from_, ovRight);
        }
        if (ovLeft) left = util::getMinValue( getWidth(), signed_);
        if (ovRight) right = util::getMaxValue( getWidth(), signed_);
        return factory_->getInteger(left, right, signed_);
    }
}

Domain::Ptr IntegerInterval::mul(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<IntegerInterval>(other.get());
    ASSERT(value, "Nullptr in interval");
    ASSERT(this->getWidth() == value->getWidth(), "Multiplying two intervals of different format");

    if (this->isBottom() || value->isBottom()) {
        return factory_->getInteger(getWidth(), signed_);
    } else if (this->isTop() || value->isTop()) {
        return factory_->getInteger(TOP, getWidth(), signed_);
    } else {

        bool ovFF = false, ovTF = false, ovFT = false, ovTT = false;
        llvm::APInt fromFrom, toFrom, fromTo, toTo;
        if (signed_) {
            fromFrom = from_.smul_ov(value->from_, ovFF);
            toFrom = to_.smul_ov(value->from_, ovTF);
            fromTo = from_.smul_ov(value->to_, ovFT);
            toTo = to_.smul_ov(value->to_, ovTT);
        } else {
            fromFrom = from_.umul_ov(value->from_, ovFF);
            toFrom = to_.umul_ov(value->from_, ovTF);
            fromTo = from_.umul_ov(value->to_, ovFT);
            toTo = to_.umul_ov(value->to_, ovTT);
        }
        if (ovFF) fromFrom = util::getMinValue( getWidth(), signed_);
        if (ovTF) toFrom = util::getMaxValue( getWidth(), signed_);
        if (ovFT) fromTo = util::getMinValue( getWidth(), signed_);
        if (ovTT) toTo = util::getMaxValue( getWidth(), signed_);

        using namespace borealis::util;
        auto first = factory_->getInteger(min(fromFrom, toFrom, signed_), max(fromFrom, toFrom, signed_), signed_);
        auto second = factory_->getInteger(min(fromTo, toTo, signed_), max(fromTo, toTo, signed_), signed_);

        return first->join(second);
    }
}

#define DIV_OPERATION(OPER) \
    auto&& value = llvm::dyn_cast<IntegerInterval>(other.get()); \
    ASSERT(value, "Nullptr in interval"); \
    ASSERT(this->getWidth() == value->getWidth(), "Dividing two intervals of different format"); \
     \
    if (isBottom() || value->isBottom()) { \
        return factory_->getInteger(getWidth(), signed_); \
    } else if (this->isTop() || value->isTop()) { \
        return factory_->getInteger(TOP, getWidth(), signed_); \
    } else { \
        \
        if (value->isConstant(0)) { \
            return factory_->getInteger(TOP, getWidth(), signed_); \
        } else { \
         \
            using namespace borealis::util; \
            Domain::Ptr first, second; \
            if (value->from_ == 0) { \
                first = factory_->getInteger(llvm::APInt( getWidth(), 0, false), to_, false); \
            } else { \
                first = factory_->getInteger(min(from_.OPER(value->from_), to_.OPER(value->from_), signed_), \
                                             max(from_.OPER(value->from_), to_.OPER(value->from_), signed_), \
                                             signed_); \
            } \
            if (value->to_ == 0) { \
                second = factory_->getInteger(llvm::APInt( getWidth(), 0, false), to_, false); \
            } else { \
                second = factory_->getInteger(min(from_.OPER(value->to_), to_.OPER(value->to_), signed_), \
                                              max(from_.OPER(value->to_), to_.OPER(value->to_), signed_), \
                                              signed_); \
            } \
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
    auto&& value = llvm::dyn_cast<IntegerInterval>(other.get()); \
    ASSERT(value, "Nullptr in shl"); \
     \
    if (isBottom() || value->isBottom()) { \
        return factory_->getInteger(getWidth(), signed_); \
    } else if (this->isTop() || value->isTop()) { \
        return factory_->getInteger(TOP, getWidth(), signed_); \
    } else { \
        using namespace borealis::util; \
        Domain::Ptr first = factory_->getInteger(min(from_.OPER(value->from_), to_.OPER(value->from_), signed_), \
                                                 max(from_.OPER(value->from_), to_.OPER(value->from_), signed_), \
                                                 signed_); \
        Domain::Ptr second = factory_->getInteger(min(from_.OPER(value->to_), to_.OPER(value->to_), signed_), \
                                                  max(from_.OPER(value->to_), to_.OPER(value->to_), signed_), \
                                                  signed_); \
         \
        return first->join(second); \
    } \

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
        return factory_->getInteger(getWidth(), signed_);

    return factory_->getInteger(TOP, getWidth(), signed_);
}

Domain::Ptr IntegerInterval::bOr(Domain::Ptr other) const {
    if (this->isBottom() || other->isBottom())
        return factory_->getInteger(getWidth(), signed_);

    return factory_->getInteger(TOP, getWidth(), signed_);
}

Domain::Ptr IntegerInterval::bXor(Domain::Ptr other) const {
    if (this->isBottom() || other->isBottom())
        return factory_->getInteger(getWidth(), signed_);

    return factory_->getInteger(TOP, getWidth(), signed_);
}

Domain::Ptr IntegerInterval::trunc(const llvm::Type& type) const {
    ASSERT(type.isIntegerTy(), "Non-integer type in trunc");
    auto&& intType = llvm::cast<llvm::IntegerType>(&type);

    if (isBottom()) return factory_->getInteger(BOTTOM, intType->getBitWidth(), signed_);
    if (isTop()) return factory_->getInteger(TOP, intType->getBitWidth(), signed_);

    auto maxTruncVal = util::getMaxValue(intType->getBitWidth(), signed_);
    llvm::APInt extendedTo = signed_ ? maxTruncVal.sext( getWidth()) : maxTruncVal.zext( getWidth());
    if (util::le(to_, extendedTo)) {
        auto&& newFrom = from_.trunc(intType->getBitWidth());
        auto&& newTo = to_.trunc(intType->getBitWidth());
        return factory_->getInteger(newFrom, newTo, signed_);

    } else {
        return factory_->getInteger(TOP, intType->getBitWidth(), signed_);
    }
}

Domain::Ptr IntegerInterval::zext(const llvm::Type& type) const {
    ASSERT(type.isIntegerTy(), "Non-integer type in zext");
    auto&& intType = llvm::cast<llvm::IntegerType>(&type);

    if (isBottom()) return factory_->getInteger(BOTTOM, intType->getBitWidth(), signed_);

    auto&& newFrom = from_.zext(intType->getBitWidth());
    auto&& newTo = to_.zext(intType->getBitWidth());
    return factory_->getInteger(llvm::APInt(intType->getBitWidth(), 0, signed_),
                                util::max(newFrom, newTo, signed_),
                                signed_);
}

Domain::Ptr IntegerInterval::sext(const llvm::Type& type) const {
    ASSERT(type.isIntegerTy(), "Non-integer type in sext");
    auto&& intType = llvm::cast<llvm::IntegerType>(&type);

    if (isBottom()) return factory_->getInteger(BOTTOM, intType->getBitWidth(), signed_);

    auto&& newFrom = from_.sext(intType->getBitWidth());
    auto&& newTo = to_.sext(intType->getBitWidth());
    return factory_->getInteger(newFrom, newTo, signed_);
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
    llvm::APInt newFrom = from_, newTo = to_; \
    if (width <  getWidth()) { \
        newFrom = from_.trunc(width); \
        newTo = to_.trunc(width); \
    } else if (width >  getWidth()) { \
        newFrom = from_.OPER(width); \
        newTo = to_.OPER(width); \
    } \
    llvm::APFloat from(newSemantics, newFrom), to(newSemantics, newTo); \
    return factory_->getFloat(from, to);

Domain::Ptr IntegerInterval::uitofp(const llvm::Type& type) const {
    INT_TO_FP(zext)
}

Domain::Ptr IntegerInterval::sitofp(const llvm::Type& type) const {
    INT_TO_FP(sext)
}

Domain::Ptr IntegerInterval::inttoptr(const llvm::Type&) const {
    return factory_->getPointer(TOP);
}

Domain::Ptr IntegerInterval::bitcast(const llvm::Type& type) const {
    if (type.isIntegerTy()) {
        auto&& intType = llvm::dyn_cast<llvm::IntegerType>(&type);
        auto&& from = (intType->getBitWidth() < getWidth()) ?
                      from_.getHiBits(intType->getBitWidth()) :
                      from_;
        auto&& to = (intType->getBitWidth() < getWidth()) ?
                    to_.getHiBits(intType->getBitWidth()) :
                    to_;
        return factory_->getInteger(from, to, signed_);
    } else if (type.isFloatingPointTy()) {
        auto&& from = llvm::APFloat(from_.bitsToDouble());
        auto&& to = llvm::APFloat(to_.bitsToDouble());
        return factory_->getFloat(from, to);
    } else {
        UNREACHABLE("Bitcast to unknown type");
    }
}

Domain::Ptr IntegerInterval::icmp(Domain::Ptr other, llvm::CmpInst::Predicate operation) const {
    auto&& getBool = [&] (bool val) -> Domain::Ptr {
        llvm::APInt retval(1, 0, false);
        if (val) retval = 1;
        else retval = 0;
        return factory_->getInteger(retval);
    };

    if (this->isBottom() || other->isBottom()) {
        return factory_->getInteger(1);
    } else if (this->isTop() || other->isTop()) {
        return factory_->getInteger(TOP, 1);
    }

    auto&& value = llvm::dyn_cast<IntegerInterval>(other.get());
    ASSERT(value, "Nullptr in interval");
    ASSERT(this->getWidth() == value->getWidth(), "Joining two intervals of different format");

    switch (operation) {
        case llvm::CmpInst::ICMP_EQ:
            if (this->isConstant() && value->isConstant() && from_ == value->from_) {
                return getBool(true);
            } else if (this->intersects(value)) {
                return factory_->getInteger(TOP, 1);
            } else {
                return getBool(false);
            }

        case llvm::CmpInst::ICMP_NE:
            if (this->isConstant() && value->isConstant() && from_ == value->from_) {
                return getBool(false);
            } else if (this->intersects(value)) {
                return factory_->getInteger(TOP, 1);
            } else {
                return getBool(true);
            }

        case llvm::CmpInst::ICMP_SGE:
            if (from_.sge(value->to_)) {
                return getBool(true);
            } else if (to_.slt(value->from_)) {
                return getBool(false);
            } else {
                return factory_->getInteger(TOP, 1);
            }

        case llvm::CmpInst::ICMP_SGT:
            if (from_.sgt(value->to_)) {
                return getBool(true);
            } else if (to_.sle(value->from_)) {
                return getBool(false);
            } else {
                return factory_->getInteger(TOP, 1);
            }

        case llvm::CmpInst::ICMP_SLE:
            if (to_.sle(value->from_)) {
                return getBool(true);
            } else if (from_.sgt(value->to_)) {
                return getBool(false);
            } else {
                return factory_->getInteger(TOP, 1);
            }

        case llvm::CmpInst::ICMP_SLT:
            if (to_.slt(value->from_)) {
                return getBool(true);
            } else if (from_.sge(value->to_)) {
                return getBool(false);
            } else {
                return factory_->getInteger(TOP, 1);
            }

        case llvm::CmpInst::ICMP_UGE:
            if (from_.uge(value->to_)) {
                return getBool(true);
            } else if (to_.ult(value->from_)) {
                return getBool(false);
            } else {
                return factory_->getInteger(TOP, 1);
            }

        case llvm::CmpInst::ICMP_UGT:
            if (from_.ugt(value->to_)) {
                return getBool(true);
            } else if (to_.ule(value->from_)) {
                return getBool(false);
            } else {
                return factory_->getInteger(TOP, 1);
            }

        case llvm::CmpInst::ICMP_ULE:
            if (to_.ule(value->from_)) {
                return getBool(true);
            } else if (from_.ugt(value->to_)) {
                return getBool(false);
            } else {
                return factory_->getInteger(TOP, 1);
            }

        case llvm::CmpInst::ICMP_ULT:
            if (to_.ult(value->from_)) {
                return getBool(true);
            } else if (from_.uge(value->to_)) {
                return getBool(false);
            } else {
                return factory_->getInteger(TOP, 1);
            }

        default:
            UNREACHABLE("Unknown operation in icmp");
    }
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"