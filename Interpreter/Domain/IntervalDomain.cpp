//
// Created by abdullin on 2/2/17.
//

#include "IntervalDomain.h"

#include "Util/hash.hpp"
#include "Util/macros.h"

#include "Util.hpp"

namespace borealis {
namespace absint {

IntervalDomain::IntervalDomain(unsigned width, bool isSigned) : Domain(BOTTOM, IntegerInterval),
                                                                width_(width),
                                                                from_(width_, not isSigned),
                                                                to_(width_, not isSigned) {}

IntervalDomain::IntervalDomain(const llvm::APSInt &constant) : Domain(VALUE, IntegerInterval),
                                                               width_(constant.getBitWidth()),
                                                               from_(constant),
                                                               to_(constant) {}

IntervalDomain::IntervalDomain(const llvm::APSInt &from, const llvm::APSInt &to) : Domain(VALUE, IntegerInterval),
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


IntervalDomain::IntervalDomain(const IntervalDomain &interval) : Domain(interval.value_, IntegerInterval),
                                                                 width_(interval.width_),
                                                                 from_(interval.from_),
                                                                 to_(interval.to_) {}

void IntervalDomain::setTop() {
    Domain::setTop();
    from_ = llvm::APSInt::getMinValue(width_, not isSigned());
    to_ = llvm::APSInt::getMaxValue(width_, not isSigned());
}

Domain::Ptr IntervalDomain::join(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<IntervalDomain>(other.get());
    ASSERT(value, "Nullptr in interval join");
    ASSERT(equalFormat(*value), "Joining two intervals of different format");

    if (value->isBottom()) {
        return Domain::Ptr{ clone() };
    } else if (this->isBottom()) {
        return Domain::Ptr{ value->clone() };
    } else {
        auto newFrom = (from_ < value->from_) ? from_ : value->from_;
        auto newTo = (to_ < value->to_) ? value->to_ : to_;
        return std::make_shared<IntervalDomain>(IntervalDomain(newFrom, newTo));
    }
}

Domain::Ptr IntervalDomain::meet(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<IntervalDomain>(other.get());
    ASSERT(value, "Nullptr in interval join");
    ASSERT(equalFormat(*value), "Joining two intervals of different format");

    if (this->isBottom() || value->isBottom()) {
        return clone()->shared_from_this();
    } else {
        auto left = (from_ < value->from_) ? value->from_ : from_;
        auto right = (to_ < value->to_) ? to_ : value->to_;
        return (left > right) ?
               clone()->shared_from_this() :
               std::make_shared<IntervalDomain>(IntervalDomain(left, right));
    }
}

unsigned IntervalDomain::getWidth() const { return width_; }
bool IntervalDomain::isSigned() const { return from_.isSigned(); }
bool IntervalDomain::isConstant() const { return from_ == to_; }
const llvm::APSInt& IntervalDomain::from() const { return from_; }
const llvm::APSInt& IntervalDomain::to() const { return to_; }

/// Assumes that both intervals have value
bool IntervalDomain::intersects(const IntervalDomain* other) const {
    ASSERT(this->isValue() && other->isValue(), "Not value intervals");

    if (from_ < other->from_ && other->from_ < to_) {
        return true;
    } else if (other->from_ < to_ && to_ < other->to_) {
        return true;
    }
    return false;
}

bool IntervalDomain::isCorrect() const {
    return from_ <= to_;
}

bool IntervalDomain::equalFormat(const IntervalDomain &other) const {
    return this->getWidth() == other.getWidth()
           && this->isSigned() == other.isSigned();
}

bool IntervalDomain::equals(const Domain *other) const {
    auto&& value = llvm::dyn_cast<IntervalDomain>(other);
    if (not value) return false;
    return this->width_ == value->width_ &&
            this->from_ == value->from_ &&
            this->to_ == value->to_;
}

bool IntervalDomain::operator<(const Domain &other) const {
    auto&& value = llvm::dyn_cast<IntervalDomain>(&other);
    ASSERT(value, "Comparing domains of different type");
    return value->from_ < this->from_ && this->to_ < value->to_;
}

bool IntervalDomain::classof(const Domain *other) {
    return other->getType() == Type::IntegerInterval;
}

size_t IntervalDomain::hashCode() const {
    return util::hash::simple_hash_value(value_, type_, width_,
                                         from_.toString(10, isSigned()),
                                         to_.toString(10, isSigned()));
}

std::string IntervalDomain::toString() const {
    if (isBottom()) return "[]";
    return "[" + from_.toString(10, isSigned()) + "," + to_.toString(10, isSigned()) + "]";
}

Domain *IntervalDomain::clone() const {
    return new IntervalDomain(*this);
}

Domain::Ptr IntervalDomain::add(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<IntervalDomain>(other.get());
    ASSERT(value, "Nullptr in interval");
    ASSERT(equalFormat(*value), "Joining two intervals of different format");

    if (this->isBottom() || value->isBottom()) {
        return std::make_shared<IntervalDomain>(IntervalDomain(this->width_, this->isSigned()));
    } else {
        auto&& left = from_ + value->from_;
        auto&& right = to_ + value->to_;
        auto&& newInterval = std::make_shared<IntervalDomain>(left, right);
        ASSERT(newInterval->isCorrect(), "Error because of overflow");

        return newInterval;
    }
}

Domain::Ptr IntervalDomain::sub(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<IntervalDomain>(other.get());
    ASSERT(value, "Nullptr in interval");
    ASSERT(equalFormat(*value), "Joining two intervals of different format");

    if (this->isBottom() || value->isBottom()) {
        return std::make_shared<IntervalDomain>(IntervalDomain(this->width_, this->isSigned()));
    } else {
        auto&& left = from_ - value->to_;
        auto&& right = to_ - value->from_;
        auto&& newInterval = std::make_shared<IntervalDomain>(left, right);
        ASSERT(newInterval->isCorrect(), "Error because of overflow");

        return newInterval;
    }
}

Domain::Ptr IntervalDomain::mul(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<IntervalDomain>(other.get());
    ASSERT(value, "Nullptr in interval");
    ASSERT(equalFormat(*value), "Joining two intervals of different format");

    if (this->isBottom() || value->isBottom()) {
        return std::make_shared<IntervalDomain>(IntervalDomain(this->width_, this->isSigned()));
    } else {

        using namespace borealis::util;
        auto first = Domain::Ptr{ new IntervalDomain(min(from_ * value->from_, to_ * value->from_),
                                                     max(from_ * value->from_, to_ * value->from_)) };
        auto second = Domain::Ptr{ new IntervalDomain(min(from_ * value->to_, to_ * value->to_),
                                                      max(from_ * value->to_, to_ * value->to_)) };

        auto newInterval = first->join(second);
        ASSERT(newInterval->isCorrect(), "error because of overflow");
        return newInterval;
    }
}

Domain::Ptr IntervalDomain::udiv(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<IntervalDomain>(other.get());
    ASSERT(value, "Nullptr in interval");
    ASSERT(equalFormat(*value), "Joining two intervals of different format");

    if (isBottom() || value->isBottom()) {
        return std::make_shared<IntervalDomain>(IntervalDomain(this->width_, this->isSigned()));
    } else {

        if (value->from_ == 0 && value->to_ == 0) {
            auto&& result = std::make_shared<IntervalDomain>(IntervalDomain(this->width_, this->isSigned()));
            result->setTop();
            return result;
        } else {

            using namespace borealis::util;
            auto first = Domain::Ptr{ new IntervalDomain(umin(from_.udiv(value->from_), to_.udiv(value->from_)),
                                                         umax(from_.udiv(value->from_), to_.udiv(value->from_))) };
            auto second = Domain::Ptr{ new IntervalDomain(umin(from_.udiv(value->to_), to_.udiv(value->to_)),
                                                          umax(from_.udiv(value->to_), to_.udiv(value->to_))) };
            auto newInterval = first->join(second);
            // TODO: do we need this?
            ASSERT(newInterval->isCorrect(), "error because of overflow");
            return newInterval;
        }
    }
}

Domain::Ptr IntervalDomain::sdiv(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<IntervalDomain>(other.get());
    ASSERT(value, "Nullptr in interval");
    ASSERT(equalFormat(*value), "Joining two intervals of different format");

    if (isBottom() || value->isBottom()) {
        return std::make_shared<IntervalDomain>(IntervalDomain(this->width_, this->isSigned()));
    } else {

        if (value->from_ == 0 && value->to_ == 0) {
            auto&& result = std::make_shared<IntervalDomain>(IntervalDomain(this->width_, this->isSigned()));
            result->setTop();
            return result->shared_from_this();
        } else {

            using namespace borealis::util;
            auto first = Domain::Ptr{ new IntervalDomain(smin(from_.sdiv(value->from_), to_.sdiv(value->from_)),
                                                         smax(from_.sdiv(value->from_), to_.sdiv(value->from_))) };
            auto second = Domain::Ptr{ new IntervalDomain(smin(from_.sdiv(value->to_), to_.sdiv(value->to_)),
                                                          smax(from_.sdiv(value->to_), to_.sdiv(value->to_))) };
            auto newInterval = first->join(second);
            // TODO: do we need this?
            ASSERT(newInterval->isCorrect(), "error because of overflow");
            return newInterval;
        }
    }
}

Domain::Ptr IntervalDomain::urem(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<IntervalDomain>(other.get());
    ASSERT(value, "Nullptr in interval");
    ASSERT(equalFormat(*value), "Joining two intervals of different format");

    if (isBottom() || value->isBottom()) {
        return std::make_shared<IntervalDomain>(IntervalDomain(this->width_, this->isSigned()));
    } else if (value->isTop()) {
        auto&& result = std::make_shared<IntervalDomain>(IntervalDomain(this->width_, this->isSigned()));
        result->setTop();
        return result;
    }

    if (value->isConstant() && value->from_ == 0) {
        auto&& result = std::make_shared<IntervalDomain>(IntervalDomain(this->width_, this->isSigned()));
        result->setTop();
        return result;
    }



    return Domain::urem(other);
}

Domain::Ptr IntervalDomain::srem(Domain::Ptr other) const {
    return Domain::srem(other);
}

Domain::Ptr IntervalDomain::shl(Domain::Ptr other) const {
    auto&& result = std::make_shared<IntervalDomain>(IntervalDomain(width_, isSigned()));
    if (this->isBottom() || other->isBottom())
        return result;

    result->setTop();
    return result;
}

Domain::Ptr IntervalDomain::lshr(Domain::Ptr other) const {
    auto&& result = std::make_shared<IntervalDomain>(IntervalDomain(width_, isSigned()));
    if (this->isBottom() || other->isBottom())
        return result;

    result->setTop();
    return result;
}

Domain::Ptr IntervalDomain::ashr(Domain::Ptr other) const {
    auto&& result = std::make_shared<IntervalDomain>(IntervalDomain(width_, isSigned()));
    if (this->isBottom() || other->isBottom())
        return result;

    result->setTop();
    return result;
}

Domain::Ptr IntervalDomain::bAnd(Domain::Ptr other) const {
    auto&& result = std::make_shared<IntervalDomain>(IntervalDomain(width_, isSigned()));
    if (this->isBottom() || other->isBottom())
        return result;

    result->setTop();
    return result;
}

Domain::Ptr IntervalDomain::bOr(Domain::Ptr other) const {
    auto&& result = std::make_shared<IntervalDomain>(IntervalDomain(width_, isSigned()));
    if (this->isBottom() || other->isBottom())
        return result;

    result->setTop();
    return result;
}

Domain::Ptr IntervalDomain::bXor(Domain::Ptr other) const {
    auto&& result = std::make_shared<IntervalDomain>(IntervalDomain(width_, isSigned()));
    if (this->isBottom() || other->isBottom())
        return result;

    result->setTop();
    return result;
}

Domain::Ptr IntervalDomain::trunc(const llvm::Type& type) const {
    return Domain::trunc(type);
}

Domain::Ptr IntervalDomain::zext(const llvm::Type& type) const {
    return Domain::zext(type);
}

Domain::Ptr IntervalDomain::sext(const llvm::Type& type) const {
    return Domain::sext(type);
}

Domain::Ptr IntervalDomain::uitofp(const llvm::Type& type) const {
    return Domain::uitofp(type);
}

Domain::Ptr IntervalDomain::sitofp(const llvm::Type& type) const {
    return Domain::sitofp(type);
}

Domain::Ptr IntervalDomain::inttoptr(const llvm::Type& type) const {
    return Domain::inttoptr(type);
}

Domain::Ptr IntervalDomain::icmp(Domain::Ptr other, llvm::CmpInst::Predicate operation) const {
    auto&& getBool = [] (bool val) -> IntervalDomain::Ptr {
        llvm::APSInt retval(1, true);
        if (val) retval = 1;
        else retval = 0;
        return std::make_shared<IntervalDomain>(IntervalDomain(retval));
    };

    auto&& getTop = [] () -> IntervalDomain::Ptr {
        auto&& result = std::make_shared<IntervalDomain>(IntervalDomain(1, false));
        result->setTop();
        return result;
    };

    if (this->isBottom() || other->isBottom()) {
        return std::make_shared<IntervalDomain>(IntervalDomain(1, false));  // bottom by default
    } else if (this->isTop() || other->isTop()) {
        return getTop();
    }

    auto&& value = llvm::dyn_cast<IntervalDomain>(other.get());
    ASSERT(value, "Nullptr in interval");
    ASSERT(equalFormat(*value), "Joining two intervals of different format");

    switch (operation) {
        case llvm::CmpInst::ICMP_EQ:
            if (this->isConstant() && value->isConstant() && from_ == value->from_) {
                return getBool(true);
            } else if (this->intersects(value)) {
                return getTop();
            } else {
                return getBool(false);
            }

        case llvm::CmpInst::ICMP_NE:
            if (this->isConstant() && value->isConstant() && from_ == value->from_) {
                return getBool(false);
            } else if (this->intersects(value)) {
                return getTop();
            } else {
                return getBool(true);
            }

        case llvm::CmpInst::ICMP_SGE:
            if (from_.sge(value->to_)) {
                return getBool(true);
            } else if (to_.slt(value->from_)) {
                return getBool(false);
            } else {
                return getTop();
            }

        case llvm::CmpInst::ICMP_SGT:
            if (from_.sgt(value->to_)) {
                return getBool(true);
            } else if (to_.sle(value->from_)) {
                return getBool(false);
            } else {
                return getTop();
            }

        case llvm::CmpInst::ICMP_SLE:
            if (to_.sle(value->from_)) {
                return getBool(true);
            } else if (from_.sgt(value->to_)) {
                return getBool(false);
            } else {
                return getTop();
            }

        case llvm::CmpInst::ICMP_SLT:
            if (to_.slt(value->from_)) {
                return getBool(true);
            } else if (from_.sge(value->to_)) {
                return getBool(false);
            } else {
                return getTop();
            }

        case llvm::CmpInst::ICMP_UGE:
            if (from_.uge(value->to_)) {
                return getBool(true);
            } else if (to_.ult(value->from_)) {
                return getBool(false);
            } else {
                return getTop();
            }

        case llvm::CmpInst::ICMP_UGT:
            if (from_.ugt(value->to_)) {
                return getBool(true);
            } else if (to_.ule(value->from_)) {
                return getBool(false);
            } else {
                return getTop();
            }

        case llvm::CmpInst::ICMP_ULE:
            if (to_.ule(value->from_)) {
                return getBool(true);
            } else if (from_.ugt(value->to_)) {
                return getBool(false);
            } else {
                return getTop();
            }

        case llvm::CmpInst::ICMP_ULT:
            if (to_.ult(value->from_)) {
                return getBool(true);
            } else if (from_.uge(value->to_)) {
                return getBool(false);
            } else {
                return getTop();
            }

        default:
            UNREACHABLE("Unknown operation in icmp");
    }
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"