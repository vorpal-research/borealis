//
// Created by abdullin on 2/13/19.
//

#ifndef BOREALIS_DOUBLEINTERVAL_HPP
#define BOREALIS_DOUBLEINTERVAL_HPP

#include "Interval.hpp"

#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {

template <typename N1, typename N2>
class DoubleInterval : public AbstractDomain {
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

    void normalize() {
        if (first_->isTop() || second_->isTop()) setTop();
        else if (first_->isBottom() || second_->isBottom()) setBottom();
    }

    template <typename N>
    const Interval<N>* unwrapInterval(Ptr domain) const {
        auto* raw = llvm::dyn_cast<Interval<N>>(domain.get());
        ASSERTC(raw);

        return raw;
    }

public:
    struct TopTag {};
    struct BottomTag {};

    DoubleInterval(TopTag, const Caster1* c1, const Caster2* c2) :
            AbstractDomain(class_tag(*this)), first_(Interval1::top(c1)), second_(Interval2::top(c2)) {}
    DoubleInterval(BottomTag, const Caster1* c1, const Caster2* c2) :
            AbstractDomain(class_tag(*this)), first_(Interval1::bottom(c1)), second_(Interval2::bottom(c2)) {}

    DoubleInterval() : DoubleInterval(TopTag{}) {}
    DoubleInterval(int n, const Caster1* c1, const Caster2* c2) :
            AbstractDomain(class_tag(*this)),
            first_(Interval1::constant(n, c1)),
            second_(Interval2::constant(n, c2)) {
        normalize();
    }

    DoubleInterval(int from, int to, const Caster1* c1, const Caster2* c2) :
        AbstractDomain(class_tag(*this)),
        first_(std::make_shared<Interval1>(from, to, c1)),
        second_(std::make_shared<Interval2>(from, to, c2)) {
        normalize();
    }

    DoubleInterval(const N1& n1, const N2 n2, const Caster1* c1, const Caster2* c2) :
            AbstractDomain(class_tag(*this)),
            first_(std::make_shared<Interval1>(n1, c1)),
            second_(std::make_shared<Interval2>(n2, c2)) {
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

    static Ptr top(const Caster1* c1, const Caster2* c2) { return std::make_shared<Self>(TopTag{}, c1, c2); }
    static Ptr bottom(const Caster1* c1, const Caster2* c2) { return std::make_shared<Self>(BottomTag{}, c1, c2); }
    static Ptr constant(int constant, const Caster1* c1, const Caster2* c2) { return std::make_shared<Self>(constant, c1, c2); }
    static Ptr constant(long constant, const Caster1* c1, const Caster2* c2) { return std::make_shared<Self>(constant, c1, c2); }
    static Ptr constant(double constant, const Caster1* c1, const Caster2* c2) { return constant((*c1)(constant), (*c2)(constant), c1, c2); }
    static Ptr constant(size_t constant, const Caster1* c1, const Caster2* c2) { return constant((*c1)(constant), (*c2)(constant), c1, c2); }
    static Ptr constant(const N1& n1, const N2& n2, const Caster1* c1, const Caster2* c2) { return std::make_shared<Self>(n1, n2, c1, c2); }

    Ptr clone() const override {
        return std::make_shared<Self>(*this);
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

    void setTop() override {
        first_->setTop();
        second_->setTop();
    }

    void setBottom() override {
        first_->setBottom();
        second_->setBottom();
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
        auto* otherRaw = unwrap(other);

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
            return std::make_shared<Self>(*otherRaw);
        } else if (other->isBottom()) {
            return std::make_shared<Self>(*this);
        } else {
            return std::make_shared<Self>(this->first_->join(otherRaw->first_), this->second_->join(otherRaw->second_));
        }
    }

    void joinWith(ConstPtr other) override {
        this->operator=(*unwrap(join(other)));
    }

    Ptr meet(ConstPtr other) const override {
        auto* otherRaw = unwrap(other);

        if (this->isBottom() || otherRaw->isBottom()) {
            auto* tf = unwrapInterval<N1>(this->first_);
            auto* ts = unwrapInterval<N2>(this->second_);

            return bottom(tf->caster(), ts->caster());
        } else {
            return std::make_shared<Self>(this->first_->meet(otherRaw->first_), this->second_->meet(otherRaw->second_));
        }
    }

    void meetWith(ConstPtr other) override {
        this->operator=(*unwrap(meet(other)));
    }

    Ptr widen(ConstPtr other) const override {
        auto* otherRaw = unwrap(other);

        if (this->isBottom()) {
            return std::make_shared<Self>(*otherRaw);
        } else if (other->isBottom()) {
            return std::make_shared<Self>(*this);
        } else {
            return std::make_shared<Self>(this->first_->widen(otherRaw->first_), this->second_->widen(otherRaw->second_));
        }
    }

    void widenWith(AbstractDomain::ConstPtr other) override {
        this->operator=(*unwrap(widen(other)));
    }

    size_t hashCode() const override {
        return util::hash::defaultHasher()(first_, second_);
    }

    std::string toString() const override {
        std::ostringstream ss;
        ss << "[" << first_->toString() << ", " << second_->toString() << "]";
        return ss.str();
    }

    Ptr apply(llvm::ArithType opcode, ConstPtr other) const override {
        auto* otherRaw = unwrap(other);

        return std::make_shared<Self>(first_->apply(opcode, otherRaw->first_), second_->apply(opcode, otherRaw->second_));
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
        return { std::make_shared<Self>(fsplit.true_, ssplit.true_), std::make_shared<Self>(fsplit.false_, ssplit.false_) };
    }

    Split splitByLess(ConstPtr other) const override {
        if (this->isBottom() || other->isBottom()) return { clone(), clone() };
        auto* otherRaw = unwrap(other);

        auto&& fsplit = this->first_->splitByLess(otherRaw->first_);
        auto&& ssplit = this->second_->splitByLess(otherRaw->second_);
        return { std::make_shared<Self>(fsplit.true_, ssplit.true_), std::make_shared<Self>(fsplit.false_, ssplit.false_) };
    }

};

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"

#endif //BOREALIS_DOUBLEINTERVAL_HPP
