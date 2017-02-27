//
// Created by abdullin on 2/2/17.
//

#include "DomainFactory.h"
#include "IntegerInterval.h"

#include "Util.hpp"
#include "Util/hash.hpp"
#include "Util/sayonara.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

IntegerInterval::IntegerInterval(const DomainFactory* factory, unsigned width) :
        Domain(BOTTOM, Type::IntegerInterval, factory),
        width_(width),
        from_(width, 0, false),
        to_(width_, 0, false) {}


IntegerInterval::IntegerInterval(Domain::Value value, const DomainFactory* factory, unsigned width) :
        Domain(value, Type::IntegerInterval, factory),
        width_(width),
        from_(width, 0, false),
        to_(width_, 0, false) {
    if (value == TOP) setTop();
}

IntegerInterval::IntegerInterval(const DomainFactory* factory, const llvm::APInt& constant) :
        Domain(VALUE, Type::IntegerInterval, factory),
        width_(constant.getBitWidth()),
        from_(constant),
        to_(constant) {
}

IntegerInterval::IntegerInterval(const DomainFactory* factory, const llvm::APInt& from, const llvm::APInt& to) :
        Domain(VALUE, Type::IntegerInterval, factory),
        width_(from.getBitWidth()),
        from_(from),
        to_(to) {
    ASSERT(from.getBitWidth() == to.getBitWidth(), "Different bit width of interval bounds");
    ASSERT(isCorrect(), "Interval lower bound is greater than upper bound:" + from_.toString(10, false) + " " + to_.toString(10, false));
    if (from.isMinValue() && to.isMaxValue()) value_ = TOP;
}

IntegerInterval::IntegerInterval(const IntegerInterval &interval) :
        Domain(interval.value_, Type::IntegerInterval, interval.factory_),
        width_(interval.width_),
        from_(interval.from_),
        to_(interval.to_) {}

void IntegerInterval::setTop() {
    Domain::setTop();
    from_ = llvm::APInt::getMinValue(getWidth());
    to_ = llvm::APInt::getMaxValue(getWidth());
}

Domain::Ptr IntegerInterval::join(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<IntegerInterval>(other.get());
    ASSERT(value, "Nullptr in interval join");
    ASSERT(this->getWidth() == value->getWidth(), "Joining two intervals of different format");

    if (value->isBottom()) {
        return Domain::Ptr{ clone() };
    } else if (this->isBottom()) {
        return Domain::Ptr{ value->clone() };
    } else {
        llvm::APInt left = util::min(from_, value->from_, false);
        llvm::APInt right = util::max(to_, value->to_, false);
        return factory_->getInteger(left, right);
    }
}

Domain::Ptr IntegerInterval::meet(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<IntegerInterval>(other.get());
    ASSERT(value, "Nullptr in interval meet");
    ASSERT(this->getWidth() == value->getWidth(), "Joining two intervals of different format");

    if (this->isBottom() || value->isBottom()) {
        return Domain::Ptr{ clone() };
    } else {
        using borealis::util::lt;
        llvm::APInt left = lt(from_, value->from_, false) ? value->from_ : from_;
        llvm::APInt right = lt(to_, value->to_, false) ? to_ : value->to_;

        return util::gt(left, right, false) ?
               clone()->shared_from_this() :
               factory_->getInteger(left, right);
    }
}

Domain::Ptr IntegerInterval::widen(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<IntegerInterval>(other.get());
    ASSERT(value, "Nullptr in interval");
    ASSERT(this->getWidth() == value->getWidth(), "Widening two intervals of different format");

    if (value->isBottom()) {
        return Domain::Ptr{ clone() };
    } else if (this->isBottom()) {
        return Domain::Ptr{ value->clone() };
    } else if (this->isTop() || value->isTop()) {
        return factory_->getInteger(TOP, getWidth());
    } else if (this->equals(value)) {
        return Domain::Ptr{ clone() };
    }

    if (this->operator<(*value)) {
        return Domain::Ptr{ value->clone() };
    } else if (value->operator<(*this)) {
        return Domain::Ptr{ clone() };
    } else if (to_.ule(value->from_)) {
        auto left = from_;
        auto right = llvm::APInt::getMaxValue(getWidth());
        return factory_->getInteger(left, right);
    } else if (value->to_.ule(from_)) {
        auto left = llvm::APInt::getMinValue(getWidth());
        auto right = to_;
        return factory_->getInteger(left, right);
    }

    return factory_->getInteger(TOP, getWidth());
}

unsigned IntegerInterval::getWidth() const { return width_; }
bool IntegerInterval::isConstant() const { return from_ == to_; }
const llvm::APInt& IntegerInterval::from() const { return from_; }
const llvm::APInt& IntegerInterval::to() const { return to_; }

/// Assumes that both intervals have value
bool IntegerInterval::intersects(const IntegerInterval* other) const {
    ASSERT(this->isValue() && other->isValue(), "Not value intervals");

    if (from_.ult(other->from_) && other->from_.ult(to_)) {
        return true;
    } else if (other->from_.ult(to_) && to_.ult(other->to_)) {
        return true;
    }
    return false;
}

bool IntegerInterval::isCorrect() const {
    return from_.ule(to_);
}

bool IntegerInterval::equals(const Domain *other) const {
    auto&& value = llvm::dyn_cast<IntegerInterval>(other);
    if (not value) return false;

    if (this->isBottom() && value->isBottom()) return true;
    if (this->isTop() && value->isTop()) return true;

    return this->getWidth() == value->getWidth() &&
            this->from_.eq(value->from_) &&
            this->to_.eq(value->to_);
}

bool IntegerInterval::operator<(const Domain &other) const {
    auto&& value = llvm::dyn_cast<IntegerInterval>(&other);
    ASSERT(value, "Comparing domains of different type");

    if (value->isBottom()) return false;
    if (this->isBottom()) return true;
    if (this->isTop()) return false;

    return value->from_.ule(this->from_) && this->to_.ule(value->to_);
}

bool IntegerInterval::classof(const Domain *other) {
    return other->getType() == Type::IntegerInterval;
}

size_t IntegerInterval::hashCode() const {
    return util::hash::simple_hash_value(value_, getType(), getWidth(),
                                         // fix that
                                         from_.toString(10, false),
                                         to_.toString(10, false));
}

std::string IntegerInterval::toString() const {
    if (isBottom()) return "[]";
    return "[" + from_.toString(10, false) + ", " + to_.toString(10, false) + "]";
}

Domain *IntegerInterval::clone() const {
    return new IntegerInterval(*this);
}

Domain::Ptr IntegerInterval::add(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<IntegerInterval>(other.get());
    ASSERT(value, "Nullptr in interval");
    ASSERT(this->getWidth() == value->getWidth(), "Adding two intervals of different format");

    if (this->isBottom() || value->isBottom()) {
        return factory_->getInteger(getWidth());
    } else if (this->isTop() || value->isTop()) {
        return factory_->getInteger(TOP, getWidth());
    } else {
        bool ovLeft = false, ovRight = false;
        llvm::APInt left, right;
        left = from_.uadd_ov(value->from_, ovLeft);
        right = to_.uadd_ov(value->to_, ovRight);
        if (ovLeft) left = llvm::APInt::getMinValue(getWidth());
        if (ovRight) right = llvm::APInt::getMaxValue(getWidth());
        return factory_->getInteger(left, right);
    }
}

Domain::Ptr IntegerInterval::sub(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<IntegerInterval>(other.get());
    ASSERT(value, "Nullptr in interval");
    ASSERT(this->getWidth() == value->getWidth(), "Subtracting two intervals of different format");

    if (this->isBottom() || value->isBottom()) {
        return factory_->getInteger(getWidth());
    } else if (this->isTop() || value->isTop()) {
        return factory_->getInteger(TOP, getWidth());
    } else {
        bool ovLeft = false, ovRight = false;
        llvm::APInt left, right;
        left = from_.usub_ov(value->to_, ovLeft);
        right = to_.usub_ov(value->from_, ovRight);
        if (ovLeft) left = llvm::APInt::getMinValue(getWidth());
        if (ovRight) right = llvm::APInt::getMaxValue(getWidth());
        return factory_->getInteger(left, right);
    }
}

Domain::Ptr IntegerInterval::mul(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<IntegerInterval>(other.get());
    ASSERT(value, "Nullptr in interval");
    ASSERT(this->getWidth() == value->getWidth(), "Multiplying two intervals of different format");

    if (this->isBottom() || value->isBottom()) {
        return factory_->getInteger(getWidth());
    } else if (this->isTop() || value->isTop()) {
        return factory_->getInteger(TOP, getWidth());
    } else {

        bool ovFF = false, ovTF = false, ovFT = false, ovTT = false;
        llvm::APInt fromFrom, toFrom, fromTo, toTo;
        fromFrom = from_.umul_ov(value->from_, ovFF);
        toFrom = to_.umul_ov(value->from_, ovTF);
        fromTo = from_.umul_ov(value->to_, ovFT);
        toTo = to_.umul_ov(value->to_, ovTT);
        if (ovFF) fromFrom = llvm::APInt::getMinValue(getWidth());
        if (ovTF) toFrom = llvm::APInt::getMaxValue(getWidth());
        if (ovFT) fromTo = llvm::APInt::getMinValue(getWidth());
        if (ovTT) toTo = llvm::APInt::getMaxValue(getWidth());

        using namespace borealis::util;
        auto first = factory_->getInteger(min(fromFrom, toFrom, false), max(fromFrom, toFrom, false));
        auto second = factory_->getInteger(min(fromTo, toTo, false), max(fromTo, toTo, false));

        return first->join(second);
    }
}

Domain::Ptr IntegerInterval::udiv(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<IntegerInterval>(other.get());
    ASSERT(value, "Nullptr in interval");
    ASSERT(this->getWidth() == value->getWidth(), "Dividing two intervals of different format");

    if (isBottom() || value->isBottom()) {
        return factory_->getInteger(getWidth());
    } else if (this->isTop() || value->isTop()) {
        return factory_->getInteger(TOP, getWidth());
    } else {

        if (value->from_ == 0 && value->to_ == 0) {
            return factory_->getInteger(TOP, getWidth());
        } else {

            using namespace borealis::util;
            auto first = factory_->getInteger(min(from_.udiv(value->from_), to_.udiv(value->from_), false),
                                              max(from_.udiv(value->from_), to_.udiv(value->from_), false));
            auto second = factory_->getInteger(min(from_.udiv(value->to_), to_.udiv(value->to_), false),
                                               max(from_.udiv(value->to_), to_.udiv(value->to_), false));
            return first->join(second);
        }
    }
}

Domain::Ptr IntegerInterval::sdiv(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<IntegerInterval>(other.get());
    ASSERT(value, "Nullptr in interval");
    ASSERT(this->getWidth() == value->getWidth(), "Dividing two intervals of different format");

    if (isBottom() || value->isBottom()) {
        return factory_->getInteger(getWidth());
    } else if (this->isTop() || value->isTop()) {
        return factory_->getInteger(TOP, getWidth());
    } else {

        if (value->from_ == 0 && value->to_ == 0) {
            return factory_->getInteger(TOP, getWidth());
        } else {

            using namespace borealis::util;
            auto first = factory_->getInteger(min(from_.sdiv(value->from_), to_.sdiv(value->from_), false),
                                              max(from_.sdiv(value->from_), to_.sdiv(value->from_), false));
            auto second = factory_->getInteger(min(from_.sdiv(value->to_), to_.sdiv(value->to_), false),
                                               max(from_.sdiv(value->to_), to_.sdiv(value->to_), false));
            return first->join(second);
        }
    }
}

// TODO: implement proper rem operations
Domain::Ptr IntegerInterval::urem(Domain::Ptr other) const {
    if (this->isBottom() || other->isBottom())
        return factory_->getInteger(getWidth());

    return factory_->getInteger(TOP, getWidth());
}

Domain::Ptr IntegerInterval::srem(Domain::Ptr other) const {
    if (this->isBottom() || other->isBottom())
        return factory_->getInteger(getWidth());

    return factory_->getInteger(TOP, getWidth());
}

Domain::Ptr IntegerInterval::shl(Domain::Ptr other) const {
    if (this->isBottom() || other->isBottom())
        return factory_->getInteger(getWidth());

    return factory_->getInteger(TOP, getWidth());
}

Domain::Ptr IntegerInterval::lshr(Domain::Ptr other) const {
    if (this->isBottom() || other->isBottom())
        return factory_->getInteger(getWidth());

    return factory_->getInteger(TOP, getWidth());
}

Domain::Ptr IntegerInterval::ashr(Domain::Ptr other) const {
    if (this->isBottom() || other->isBottom())
        return factory_->getInteger(getWidth());

    return factory_->getInteger(TOP, getWidth());
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

    if (isTop()) return factory_->getInteger(TOP, intType->getBitWidth());

    auto&& newFrom = from_.trunc(intType->getBitWidth());
    auto&& newTo = to_.trunc(intType->getBitWidth());
    return factory_->getInteger(newFrom, newTo);
}

Domain::Ptr IntegerInterval::zext(const llvm::Type& type) const {
    ASSERT(type.isIntegerTy(), "Non-integer type in trunc");
    auto&& intType = llvm::cast<llvm::IntegerType>(&type);

    if (isTop()) return factory_->getInteger(TOP, intType->getBitWidth());

    auto&& newFrom = from_.zext(intType->getBitWidth());
    auto&& newTo = to_.zext(intType->getBitWidth());
    return factory_->getInteger(newFrom, newTo);
}

Domain::Ptr IntegerInterval::sext(const llvm::Type& type) const {
    ASSERT(type.isIntegerTy(), "Non-integer type in trunc");
    auto&& intType = llvm::cast<llvm::IntegerType>(&type);

    if (isTop()) return factory_->getInteger(TOP, intType->getBitWidth());

    auto&& newFrom = from_.sext(intType->getBitWidth());
    auto&& newTo = to_.sext(intType->getBitWidth());
    return factory_->getInteger(newFrom, newTo);
}

Domain::Ptr IntegerInterval::uitofp(const llvm::Type& type) const {
    ASSERT(type.isFloatingPointTy(), "Non-FP type in fptrunc");
    auto& newSemantics = util::getSemantics(type);

    llvm::APInt ufrom(from_), uto(to_);
    llvm::APFloat from(newSemantics, ufrom), to(newSemantics, uto);
    return factory_->getFloat(from, to);
}

Domain::Ptr IntegerInterval::sitofp(const llvm::Type& type) const {
    ASSERT(type.isFloatingPointTy(), "Non-FP type in fptrunc");
    auto& newSemantics = util::getSemantics(type);

    llvm::APInt sfrom(from_), sto(to_);
    llvm::APFloat from(newSemantics, sfrom), to(newSemantics, sto);
    return factory_->getFloat(from, to);
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
        return factory_->getInteger(from, to);
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