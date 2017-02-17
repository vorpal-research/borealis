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

IntegerInterval::IntegerInterval(const DomainFactory* factory, unsigned width, bool isSigned) :
        Domain(BOTTOM, Type::IntegerInterval, factory),
        width_(width),
        from_(width_, not isSigned),
        to_(width_, not isSigned) {}


IntegerInterval::IntegerInterval(Domain::Value value, const DomainFactory* factory, unsigned width, bool isSigned) :
        Domain(value, Type::IntegerInterval, factory),
        width_(width),
        from_(width_, not isSigned),
        to_(width_, not isSigned) {
    if (value == TOP) setTop();
}

IntegerInterval::IntegerInterval(const DomainFactory* factory, const llvm::APSInt &constant) :
        Domain(VALUE, Type::IntegerInterval, factory),
        width_(constant.getBitWidth()),
        from_(constant),
        to_(constant) {}

IntegerInterval::IntegerInterval(const DomainFactory* factory, const llvm::APSInt &from, const llvm::APSInt &to) :
        Domain(VALUE, Type::IntegerInterval, factory),
        width_(from.getBitWidth()),
        from_(from),
        to_(to) {
    ASSERT(from.getBitWidth() == to.getBitWidth(), "Different bit width of interval bounds");
    ASSERT(from.isSigned() == to.isSigned(), "Different signedness of interval bounds");
    ASSERT(isCorrect(), "Interval lower bound is greater than upper bound");
    if (isSigned()) {
        if (from.isMinSignedValue() && to.isMaxSignedValue()) value_ = TOP;
    } else {
        if (from.isMinValue() && to.isMaxValue()) value_ = TOP;
    }
}


IntegerInterval::IntegerInterval(const IntegerInterval &interval) :
        Domain(interval.value_, Type::IntegerInterval, interval.factory_),
        width_(interval.width_),
        from_(interval.from_),
        to_(interval.to_) {}

void IntegerInterval::setTop() {
    Domain::setTop();
    from_ = llvm::APSInt::getMinValue(getWidth(), not isSigned());
    to_ = llvm::APSInt::getMaxValue(getWidth(), not isSigned());
}

Domain::Ptr IntegerInterval::join(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<IntegerInterval>(other.get());
    ASSERT(value, "Nullptr in interval join");
    ASSERT(equalFormat(*value), "Joining two intervals of different format");

    if (value->isBottom()) {
        return Domain::Ptr{ clone() };
    } else if (this->isBottom()) {
        return Domain::Ptr{ value->clone() };
    } else {
        auto newFrom = (from_ < value->from_) ? from_ : value->from_;
        auto newTo = (to_ < value->to_) ? value->to_ : to_;
        return factory_->getInteger(newFrom, newTo);
    }
}

Domain::Ptr IntegerInterval::meet(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<IntegerInterval>(other.get());
    ASSERT(value, "Nullptr in interval join");
    ASSERT(equalFormat(*value), "Joining two intervals of different format");

    if (this->isBottom() || value->isBottom()) {
        return clone()->shared_from_this();
    } else {
        auto left = (from_ < value->from_) ? value->from_ : from_;
        auto right = (to_ < value->to_) ? to_ : value->to_;
        return (left > right) ?
               clone()->shared_from_this() :
               factory_->getInteger(left, right);
    }
}

Domain::Ptr IntegerInterval::widen(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<IntegerInterval>(other.get());
    ASSERT(value, "Nullptr in interval");
    ASSERT(equalFormat(*value), "Joining two intervals of different format");
    // TODO: add proper widening
    return factory_->getInteger(TOP, getWidth(), isSigned());
}

unsigned IntegerInterval::getWidth() const { return width_; }
bool IntegerInterval::isSigned() const { return from_.isSigned(); }
bool IntegerInterval::isConstant() const { return from_ == to_; }
const llvm::APSInt& IntegerInterval::from() const { return from_; }
const llvm::APSInt& IntegerInterval::to() const { return to_; }

/// Assumes that both intervals have value
bool IntegerInterval::intersects(const IntegerInterval* other) const {
    ASSERT(this->isValue() && other->isValue(), "Not value intervals");

    if (from_ < other->from_ && other->from_ < to_) {
        return true;
    } else if (other->from_ < to_ && to_ < other->to_) {
        return true;
    }
    return false;
}

bool IntegerInterval::isCorrect() const {
    return from_ <= to_;
}

bool IntegerInterval::equalFormat(const IntegerInterval &other) const {
    return this->getWidth() == other.getWidth()
           && this->isSigned() == other.isSigned();
}

bool IntegerInterval::equals(const Domain *other) const {
    auto&& value = llvm::dyn_cast<IntegerInterval>(other);
    if (not value) return false;
    return this->getWidth() == value->getWidth() &&
            this->from_ == value->from_ &&
            this->to_ == value->to_;
}

bool IntegerInterval::operator<(const Domain &other) const {
    auto&& value = llvm::dyn_cast<IntegerInterval>(&other);
    ASSERT(value, "Comparing domains of different type");
    return value->from_ < this->from_ && this->to_ < value->to_;
}

bool IntegerInterval::classof(const Domain *other) {
    return other->getType() == Type::IntegerInterval;
}

size_t IntegerInterval::hashCode() const {
    return util::hash::simple_hash_value(value_, getType(), getWidth(),
                                         // fix that
                                         from_.toString(10, isSigned()),
                                         to_.toString(10, isSigned()));
}

std::string IntegerInterval::toString() const {
    if (isBottom()) return "[]";
    return "[" + from_.toString(10, isSigned()) + ", " + to_.toString(10, isSigned()) + "]";
}

Domain *IntegerInterval::clone() const {
    return new IntegerInterval(*this);
}

Domain::Ptr IntegerInterval::add(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<IntegerInterval>(other.get());
    ASSERT(value, "Nullptr in interval");
    ASSERT(equalFormat(*value), "Joining two intervals of different format");

    if (this->isBottom() || value->isBottom()) {
        return factory_->getInteger(getWidth(), isSigned());
    } else if (this->isTop() || value->isTop()) {
        return factory_->getInteger(TOP, getWidth(), isSigned());
    } else {
        auto&& left = from_ + value->from_;
        auto&& right = to_ + value->to_;
        return factory_->getInteger(left, right);
    }
}

Domain::Ptr IntegerInterval::sub(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<IntegerInterval>(other.get());
    ASSERT(value, "Nullptr in interval");
    ASSERT(equalFormat(*value), "Joining two intervals of different format");

    if (this->isBottom() || value->isBottom()) {
        return factory_->getInteger(getWidth(), isSigned());
    } else if (this->isTop() || value->isTop()) {
        return factory_->getInteger(TOP, getWidth(), isSigned());
    } else {
        auto&& left = from_ - value->to_;
        auto&& right = to_ - value->from_;
        return factory_->getInteger(left, right);
    }
}

Domain::Ptr IntegerInterval::mul(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<IntegerInterval>(other.get());
    ASSERT(value, "Nullptr in interval");
    ASSERT(equalFormat(*value), "Joining two intervals of different format");

    if (this->isBottom() || value->isBottom()) {
        return factory_->getInteger(getWidth(), isSigned());
    } else if (this->isTop() || value->isTop()) {
        return factory_->getInteger(TOP, getWidth(), isSigned());
    } else {

        using namespace borealis::util;
        auto first = factory_->getInteger(min(from_ * value->from_, to_ * value->from_),
                                          max(from_ * value->from_, to_ * value->from_));
        auto second = factory_->getInteger(min(from_ * value->to_, to_ * value->to_),
                                           max(from_ * value->to_, to_ * value->to_));

        return first->join(second);
    }
}

Domain::Ptr IntegerInterval::udiv(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<IntegerInterval>(other.get());
    ASSERT(value, "Nullptr in interval");
    ASSERT(equalFormat(*value), "Joining two intervals of different format");

    if (isBottom() || value->isBottom()) {
        return factory_->getInteger(getWidth(), isSigned());
    } else if (this->isTop() || value->isTop()) {
        return factory_->getInteger(TOP, getWidth(), isSigned());
    } else {

        if (value->from_ == 0 && value->to_ == 0) {
            return factory_->getInteger(TOP, getWidth(), isSigned());
        } else {

            using namespace borealis::util;
            auto first = factory_->getInteger(umin(from_.udiv(value->from_), to_.udiv(value->from_)),
                                              umax(from_.udiv(value->from_), to_.udiv(value->from_)));
            auto second = factory_->getInteger(umin(from_.udiv(value->to_), to_.udiv(value->to_)),
                                               umax(from_.udiv(value->to_), to_.udiv(value->to_)));
            return first->join(second);
        }
    }
}

Domain::Ptr IntegerInterval::sdiv(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<IntegerInterval>(other.get());
    ASSERT(value, "Nullptr in interval");
    ASSERT(equalFormat(*value), "Joining two intervals of different format");

    if (isBottom() || value->isBottom()) {
        return factory_->getInteger(getWidth(), isSigned());
    } else if (this->isTop() || value->isTop()) {
        return factory_->getInteger(TOP, getWidth(), isSigned());
    } else {

        if (value->from_ == 0 && value->to_ == 0) {
            return factory_->getInteger(TOP, getWidth(), isSigned());
        } else {

            using namespace borealis::util;
            auto first = factory_->getInteger(smin(from_.sdiv(value->from_), to_.sdiv(value->from_)),
                                              smax(from_.sdiv(value->from_), to_.sdiv(value->from_)));
            auto second = factory_->getInteger(smin(from_.sdiv(value->to_), to_.sdiv(value->to_)),
                                               smax(from_.sdiv(value->to_), to_.sdiv(value->to_)));
            return first->join(second);
        }
    }
}

// TODO: implement proper rem operations
Domain::Ptr IntegerInterval::urem(Domain::Ptr other) const {
    if (this->isBottom() || other->isBottom())
        return factory_->getInteger(getWidth(), isSigned());

    return factory_->getInteger(TOP, getWidth(), isSigned());
}

Domain::Ptr IntegerInterval::srem(Domain::Ptr other) const {
    if (this->isBottom() || other->isBottom())
        return factory_->getInteger(getWidth(), isSigned());

    return factory_->getInteger(TOP, getWidth(), isSigned());
}

Domain::Ptr IntegerInterval::shl(Domain::Ptr other) const {
    if (this->isBottom() || other->isBottom())
        return factory_->getInteger(getWidth(), isSigned());

    return factory_->getInteger(TOP, getWidth(), isSigned());
}

Domain::Ptr IntegerInterval::lshr(Domain::Ptr other) const {
    if (this->isBottom() || other->isBottom())
        return factory_->getInteger(getWidth(), isSigned());

    return factory_->getInteger(TOP, getWidth(), isSigned());
}

Domain::Ptr IntegerInterval::ashr(Domain::Ptr other) const {
    if (this->isBottom() || other->isBottom())
        return factory_->getInteger(getWidth(), isSigned());

    return factory_->getInteger(TOP, getWidth(), isSigned());
}

Domain::Ptr IntegerInterval::bAnd(Domain::Ptr other) const {
    if (this->isBottom() || other->isBottom())
        return factory_->getInteger(getWidth(), isSigned());

    return factory_->getInteger(TOP, getWidth(), isSigned());
}

Domain::Ptr IntegerInterval::bOr(Domain::Ptr other) const {
    if (this->isBottom() || other->isBottom())
        return factory_->getInteger(getWidth(), isSigned());

    return factory_->getInteger(TOP, getWidth(), isSigned());
}

Domain::Ptr IntegerInterval::bXor(Domain::Ptr other) const {
    if (this->isBottom() || other->isBottom())
        return factory_->getInteger(getWidth(), isSigned());

    return factory_->getInteger(TOP, getWidth(), isSigned());
}

Domain::Ptr IntegerInterval::trunc(const llvm::Type& type) const {
    return Domain::trunc(type);
}

Domain::Ptr IntegerInterval::zext(const llvm::Type& type) const {
    return Domain::zext(type);
}

Domain::Ptr IntegerInterval::sext(const llvm::Type& type) const {
    return Domain::sext(type);
}

Domain::Ptr IntegerInterval::uitofp(const llvm::Type& type) const {
    return Domain::uitofp(type);
}

Domain::Ptr IntegerInterval::sitofp(const llvm::Type& type) const {
    return Domain::sitofp(type);
}

Domain::Ptr IntegerInterval::inttoptr(const llvm::Type& type) const {
    return Domain::inttoptr(type);
}

Domain::Ptr IntegerInterval::icmp(Domain::Ptr other, llvm::CmpInst::Predicate operation) const {
    auto&& getBool = [&] (bool val) -> IntegerInterval::Ptr {
        llvm::APSInt retval(1, true);
        if (val) retval = 1;
        else retval = 0;
        return factory_->getInteger(retval);
    };

    if (this->isBottom() || other->isBottom()) {
        return factory_->getInteger(1, false);
    } else if (this->isTop() || other->isTop()) {
        return factory_->getInteger(TOP, 1, false);
    }

    auto&& value = llvm::dyn_cast<IntegerInterval>(other.get());
    ASSERT(value, "Nullptr in interval");
    ASSERT(equalFormat(*value), "Joining two intervals of different format");

    switch (operation) {
        case llvm::CmpInst::ICMP_EQ:
            if (this->isConstant() && value->isConstant() && from_ == value->from_) {
                return getBool(true);
            } else if (this->intersects(value)) {
                return factory_->getInteger(TOP, 1, false);
            } else {
                return getBool(false);
            }

        case llvm::CmpInst::ICMP_NE:
            if (this->isConstant() && value->isConstant() && from_ == value->from_) {
                return getBool(false);
            } else if (this->intersects(value)) {
                return factory_->getInteger(TOP, 1, false);
            } else {
                return getBool(true);
            }

        case llvm::CmpInst::ICMP_SGE:
            if (from_.sge(value->to_)) {
                return getBool(true);
            } else if (to_.slt(value->from_)) {
                return getBool(false);
            } else {
                return factory_->getInteger(TOP, 1, false);
            }

        case llvm::CmpInst::ICMP_SGT:
            if (from_.sgt(value->to_)) {
                return getBool(true);
            } else if (to_.sle(value->from_)) {
                return getBool(false);
            } else {
                return factory_->getInteger(TOP, 1, false);
            }

        case llvm::CmpInst::ICMP_SLE:
            if (to_.sle(value->from_)) {
                return getBool(true);
            } else if (from_.sgt(value->to_)) {
                return getBool(false);
            } else {
                return factory_->getInteger(TOP, 1, false);
            }

        case llvm::CmpInst::ICMP_SLT:
            if (to_.slt(value->from_)) {
                return getBool(true);
            } else if (from_.sge(value->to_)) {
                return getBool(false);
            } else {
                return factory_->getInteger(TOP, 1, false);
            }

        case llvm::CmpInst::ICMP_UGE:
            if (from_.uge(value->to_)) {
                return getBool(true);
            } else if (to_.ult(value->from_)) {
                return getBool(false);
            } else {
                return factory_->getInteger(TOP, 1, false);
            }

        case llvm::CmpInst::ICMP_UGT:
            if (from_.ugt(value->to_)) {
                return getBool(true);
            } else if (to_.ule(value->from_)) {
                return getBool(false);
            } else {
                return factory_->getInteger(TOP, 1, false);
            }

        case llvm::CmpInst::ICMP_ULE:
            if (to_.ule(value->from_)) {
                return getBool(true);
            } else if (from_.ugt(value->to_)) {
                return getBool(false);
            } else {
                return factory_->getInteger(TOP, 1, false);
            }

        case llvm::CmpInst::ICMP_ULT:
            if (to_.ult(value->from_)) {
                return getBool(true);
            } else if (from_.uge(value->to_)) {
                return getBool(false);
            } else {
                return factory_->getInteger(TOP, 1, false);
            }

        default:
            UNREACHABLE("Unknown operation in icmp");
    }
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"