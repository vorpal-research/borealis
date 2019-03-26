//
// Created by abdullin on 2/13/19.
//

#ifndef BOREALIS_DOUBLEINTERVAL_HPP
#define BOREALIS_DOUBLEINTERVAL_HPP

#include "Interval.hpp"

#include "Util/cache.hpp"
#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {

namespace impl_ {

using DoubleIntervalID = std::pair<AbstractDomain::Ptr, AbstractDomain::Ptr>;

struct DoubleIntervalIDHash {
    std::size_t operator()(const DoubleIntervalID& id) const {
        return util::hash::defaultHasher()(id.first, id.second);
    }
};

struct DoubleIntervalIDEquals {
    bool operator()(const DoubleIntervalID& lhv, const DoubleIntervalID& rhv) const {
        return lhv.first->equals(rhv.first) && lhv.second->equals(rhv.second);
    }
};

} // namespace impl_

template <typename N1, typename N2>
class DoubleInterval : public AbstractDomain {
private:

    using IntervalCacheImpl =
            std::unordered_map<impl_::DoubleIntervalID, Ptr, impl_::DoubleIntervalIDHash, impl_::DoubleIntervalIDEquals>;

private:

    static util::cacheWImpl<impl_::DoubleIntervalID, Ptr, IntervalCacheImpl> cache_;

public:

    using Self = DoubleInterval<N1, N2>;
    using Interval1 = Interval<N1>;
    using Interval2 = Interval<N2>;

    using Caster1 = typename Interval1::CasterT;
    using Caster2 = typename Interval2::CasterT;

private:

    Ptr first_;
    Ptr second_;

private:

    static Self* unwrap(Ptr other) {
        auto* otherRaw = llvm::dyn_cast<Self>(other.get());
        ASSERTC(otherRaw);

        return otherRaw;
    }

    static const Self* unwrap(ConstPtr other) {
        auto* otherRaw = llvm::dyn_cast<const Self>(other.get());
        ASSERTC(otherRaw);

        return otherRaw;
    }

    template <typename N>
    const Interval<N>* unwrapInterval(Ptr domain) const {
        auto* raw = llvm::dyn_cast<Interval<N>>(domain.get());
        ASSERTC(raw);

        return raw;
    }

    void normalize() {
        if (first_->isTop() || second_->isTop()) {
            first_ = Interval1::top(unwrapInterval<N1>(first_)->caster());
            second_ = Interval2::top(unwrapInterval<N2>(second_)->caster());
        } else if (first_->isBottom() || second_->isBottom()) {
            first_ = Interval1::bottom(unwrapInterval<N1>(first_)->caster());
            second_ = Interval2::bottom(unwrapInterval<N2>(second_)->caster());
        }
    }

public:
    struct TopTag {};
    struct BottomTag {};

    DoubleInterval(TopTag, const Caster1* c1, const Caster2* c2) :
            AbstractDomain(class_tag(*this)), first_(Interval1::top(c1)), second_(Interval2::top(c2)) {}
    DoubleInterval(BottomTag, const Caster1* c1, const Caster2* c2) :
            AbstractDomain(class_tag(*this)), first_(Interval1::bottom(c1)), second_(Interval2::bottom(c2)) {}

    DoubleInterval(int n, const Caster1* c1, const Caster2* c2) :
            AbstractDomain(class_tag(*this)),
            first_(Interval1::constant(n, c1)),
            second_(Interval2::constant(n, c2)) {
        normalize();
    }

    DoubleInterval(int from, int to, const Caster1* c1, const Caster2* c2) :
        AbstractDomain(class_tag(*this)),
        first_(Interval1::interval(Bound<N1>(c1, from), Bound<N1>(c2, to))),
        second_(Interval2::interval(Bound<N2>(c2, from), Bound<N2>(c2, to))) {
        normalize();
    }

    DoubleInterval(const N1& n1, const N2 n2, const Caster1* c1, const Caster2* c2) :
            AbstractDomain(class_tag(*this)),
            first_(Interval1::constant(n1, c1)),
            second_(Interval2::constant(n2, c2)) {
        normalize();
    }

    DoubleInterval(Ptr sint, Ptr uint) :
            AbstractDomain(class_tag(*this)),
            first_(sint),
            second_(uint) {
        normalize();
    }

    DoubleInterval(const DoubleInterval&) = default;
    DoubleInterval(DoubleInterval&&) = default;
    DoubleInterval& operator=(const DoubleInterval& other) {
        if (this != &other) {
            this->first_ = other.first_;
            this->second_ = other.second_;
        }
        return *this;
    }
    DoubleInterval& operator=(DoubleInterval&&) = default;
    ~DoubleInterval() override = default;

    static Ptr top(const Caster1* c1, const Caster2* c2) {
        return cache_[std::make_pair(Interval1::top(c1), Interval2::top(c2))];
    }

    static Ptr bottom(const Caster1* c1, const Caster2* c2) {
        return cache_[std::make_pair(Interval1::bottom(c1), Interval2::bottom(c2))];
    }

    static Ptr constant(int constant, const Caster1* c1, const Caster2* c2) {
        return cache_[std::make_pair(Interval1::constant(constant, c1), Interval2::constant(constant, c2))];
    }

    static Ptr constant(long constant, const Caster1* c1, const Caster2* c2) {
        return cache_[std::make_pair(Interval1::constant(constant, c1), Interval2::constant(constant, c2))];
    }

    static Ptr constant(double constant, const Caster1* c1, const Caster2* c2) { return constant((*c1)(constant), (*c2)(constant), c1, c2); }
    static Ptr constant(size_t constant, const Caster1* c1, const Caster2* c2) { return constant((*c1)(constant), (*c2)(constant), c1, c2); }
    static Ptr constant(const N1& n1, const N2& n2, const Caster1* c1, const Caster2* c2) {
        return cache_[std::make_pair(Interval1::constant(n1, c1), Interval2::constant(n2, c2))];
    }
    static Ptr interval(AbstractDomain::Ptr first, AbstractDomain::Ptr second) {
        return cache_[std::make_pair(first, second)];
    }

    Ptr clone() const override {
        return const_cast<Self*>(this)->shared_from_this();
    }

    static bool classof(const Self*) {
        return true;
    }

    static bool classof(const AbstractDomain* other) {
        return other->getClassTag() == class_tag<Self>();
    }

    bool isTop() const override {
        return first_->isTop() and second_->isTop();
    }

    bool isBottom() const override {
        return first_->isBottom() and second_->isBottom();
    }

    bool isConstant() const {
        return unwrapInterval<N1>(first_)->isConstant() and unwrapInterval<N2>(second_)->isConstant();
    }

    void setTop() override {
        UNREACHABLE("Should not change interval domains");
    }

    void setBottom() override {
        UNREACHABLE("Should not change interval domains");
    }

    Ptr first() const { return first_; }

    Ptr second() const { return second_; }

    bool intersects(ConstPtr other) const {
        auto* otherRaw = unwrap(other);

        auto* tf = unwrapInterval<N1>(this->first_);
        auto* ts = unwrapInterval<N2>(this->second_);

        return tf->intersects(otherRaw->first_) || ts->intersects(otherRaw->second_);
    }

    bool leq(ConstPtr other) const override {
        auto* otherRaw = unwrap(other);

        if (this->isBottom()) {
            return true;
        } else if (other->isBottom()) {
            return false;
        } else {
            return this->first_->leq(otherRaw->first_) && this->second_->leq(otherRaw->second_);
        }
    }

    bool equals(ConstPtr other) const override {
        if (this == other.get()) return true;

        auto* otherRaw = llvm::dyn_cast<const Self>(other.get());
        if (not otherRaw) return false;

        if (this->isBottom()) {
            return other->isBottom();
        } else if (other->isBottom()) {
            return false;
        } else {
            return this->first_->equals(otherRaw->first_) && this->second_->equals(otherRaw->second_);
        }
    }

    Ptr join(ConstPtr other) const override {
        auto* otherRaw = unwrap(other);

        if (this->isBottom()) {
            return other->clone();
        } else if (other->isBottom()) {
            return clone();
        } else {
            return interval(this->first_->join(otherRaw->first_), this->second_->join(otherRaw->second_));
        }
    }


    Ptr meet(ConstPtr other) const override {
        auto* otherRaw = unwrap(other);

        if (this->isBottom() || otherRaw->isBottom()) {
            auto* tf = unwrapInterval<N1>(this->first_);
            auto* ts = unwrapInterval<N2>(this->second_);

            return bottom(tf->caster(), ts->caster());
        } else {
            return interval(this->first_->meet(otherRaw->first_), this->second_->meet(otherRaw->second_));
        }
    }

    Ptr widen(ConstPtr other) const override {
        auto* otherRaw = unwrap(other);

        if (this->isBottom()) {
            return other->clone();
        } else if (other->isBottom()) {
            return clone();
        } else {
            return interval(this->first_->widen(otherRaw->first_), this->second_->widen(otherRaw->second_));
        }
    }

    size_t hashCode() const override {
        return class_tag(*this); //return util::hash::defaultHasher()(first_, second_);
    }

    std::string toString() const override {
        std::ostringstream ss;
        ss << "[" << first_->toString() << ", " << second_->toString() << "]";
        return ss.str();
    }

    Ptr apply(llvm::ArithType opcode, ConstPtr other) const override {
        auto* otherRaw = unwrap(other);

        return interval(first_->apply(opcode, otherRaw->first_), second_->apply(opcode, otherRaw->second_));
    }

    Ptr apply(llvm::ConditionType opcode, ConstPtr other) const override {
        auto* otherRaw = unwrap(other);

        return first_->apply(opcode, otherRaw->first_)->join(second_->apply(opcode, otherRaw->second_));
    }

    Split splitByEq(ConstPtr other) const override {
        if (this->isBottom() || other->isBottom()) return { clone(), clone() };
        auto* otherRaw = unwrap(other);

        auto&& fsplit = this->first_->splitByEq(otherRaw->first_);
        auto&& ssplit = this->second_->splitByEq(otherRaw->second_);
        return { interval(fsplit.true_, ssplit.true_), interval(fsplit.false_, ssplit.false_) };
    }

    Split splitByLess(ConstPtr other) const override {
        if (this->isBottom() || other->isBottom()) return { clone(), clone() };
        auto* otherRaw = unwrap(other);

        auto&& fsplit = this->first_->splitByLess(otherRaw->first_);
        auto&& ssplit = this->second_->splitByLess(otherRaw->second_);
        return { interval(fsplit.true_, ssplit.true_), interval(fsplit.false_, ssplit.false_) };
    }

};

template <typename N1, typename N2>
util::cacheWImpl<impl_::DoubleIntervalID, AbstractDomain::Ptr, typename DoubleInterval<N1, N2>::IntervalCacheImpl>
        DoubleInterval<N1, N2>::cache_([] (const impl_::DoubleIntervalID& a) -> AbstractDomain::Ptr {
    return std::make_shared<DoubleInterval<N1, N2>>(a.first, a.second);
});

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"

#endif //BOREALIS_DOUBLEINTERVAL_HPP
