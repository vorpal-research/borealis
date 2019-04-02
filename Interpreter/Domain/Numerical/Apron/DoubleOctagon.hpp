//
// Created by abdullin on 3/22/19.
//

#ifndef BOREALIS_DOUBLEOCTAGON_HPP
#define BOREALIS_DOUBLEOCTAGON_HPP

#include "Interpreter/Domain/AbstractFactory.hpp"

#include "Interpreter/Domain/Numerical/Apron/ApronDomain.hpp"

#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
using Octagon = apron::ApronDomain<Number, Variable, VarHash, VarEquals, apron::DomainT::OCTAGON>;


template <typename N1, typename N2, typename Variable, typename VarHash, typename VarEquals>
class DoubleOctagon : public NumericalDomain<Variable> {
public:

    using Ptr = AbstractDomain::Ptr;
    using ConstPtr = AbstractDomain::ConstPtr;
    using Octagon1 = Octagon<N1, Variable, VarHash, VarEquals>;
    using Octagon2 = Octagon<N2, Variable, VarHash, VarEquals>;
    using Interval1 = typename Octagon1::IntervalT;
    using Interval2 = typename Octagon2::IntervalT;
    using DInterval = DoubleInterval<N1, N2>;
    using DNumber = std::pair<N1, N2>;
    using Caster1 = typename Octagon1::CasterT;
    using Caster2 = typename Octagon2::CasterT;
    using Self = DoubleOctagon<N1, N2, Variable, VarHash, VarEquals>;

protected:

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
    Octagon<N, Variable, VarHash, VarEquals>* unwrapOctagon(Ptr domain) const {
        auto* raw = llvm::dyn_cast<Octagon<N, Variable, VarHash, VarEquals>>(domain.get());
        ASSERTC(raw);

        return raw;
    }

    const DInterval* unwrapDInterval(Ptr other) {
        auto* otherRaw = llvm::dyn_cast<DInterval>(other.get());
        ASSERTC(otherRaw);

        return otherRaw;
    }

public:
    struct TopTag {};
    struct BottomTag {};

    DoubleOctagon(const Caster1* c1, const Caster2* c2) : DoubleOctagon(TopTag{}, c1, c2) {}

    DoubleOctagon(TopTag, const Caster1* c1, const Caster2* c2) :
            NumericalDomain<Variable>(class_tag(*this)), first_(Octagon1::top(c1)), second_(Octagon2::top(c2)) {}
    DoubleOctagon(BottomTag, const Caster1* c1, const Caster2* c2) :
            NumericalDomain<Variable>(class_tag(*this)), first_(Octagon1::bottom(c1)), second_(Octagon2::bottom(c2)) {}

    DoubleOctagon(AbstractDomain::Ptr first, AbstractDomain::Ptr second) :
            NumericalDomain<Variable>(class_tag(*this)), first_(first), second_(second) {}

    DoubleOctagon(const DoubleOctagon&) = default;
    DoubleOctagon(DoubleOctagon&&) = default;
    DoubleOctagon& operator=(const DoubleOctagon& other) {
        if (this != &other) {
            this->first_ = other.first_;
            this->second_ = other.second_;
        }
        return *this;
    }
    DoubleOctagon& operator=(DoubleOctagon&&) = default;
    ~DoubleOctagon() override = default;

    static Ptr top(const Caster1* c1, const Caster2* c2) {
        return std::make_shared<Self>(TopTag{}, c1, c2);
    }

    static Ptr bottom(const Caster1* c1, const Caster2* c2) {
        return std::make_shared<Self>(BottomTag{}, c1, c2);
    }

    static Ptr doctagon(AbstractDomain::Ptr first, AbstractDomain::Ptr second) {
        return std::make_shared<Self>(first, second);
    }

    Ptr clone() const override {
        return const_cast<Self*>(this)->shared_from_this();
    }

    bool isTop() const override { return first_->isTop() and second_->isTop(); }
    bool isBottom() const override { return first_->isBottom() and second_->isBottom(); }

    void setTop() override {
        first_->setTop();
        second_->setTop();
    }

    void setBottom() override {
        first_->setBottom();
        second_->setBottom();
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
            return doctagon(this->first_->join(otherRaw->first_), this->second_->join(otherRaw->second_));
        }
    }


    Ptr meet(ConstPtr other) const override {
        auto* otherRaw = unwrap(other);

        if (this->isBottom() || otherRaw->isBottom()) {
            auto* tf = unwrapOctagon<N1>(this->first_);
            auto* ts = unwrapOctagon<N2>(this->second_);

            return bottom(tf->caster(), ts->caster());
        } else {
            return doctagon(this->first_->meet(otherRaw->first_), this->second_->meet(otherRaw->second_));
        }
    }

    Ptr widen(ConstPtr other) const override {
        auto* otherRaw = unwrap(other);

        if (this->isBottom()) {
            return other->clone();
        } else if (other->isBottom()) {
            return clone();
        } else {
            return doctagon(this->first_->widen(otherRaw->first_), this->second_->widen(otherRaw->second_));
        }
    }

    Ptr get(Variable x) const override {
        return toInterval(x);
    }

    Ptr toInterval(Variable x) const override {
        auto first = unwrapOctagon<N1>(first_)->toInterval(x);
        auto second = unwrapOctagon<N2>(second_)->toInterval(x);
        return (first and second) ? DInterval::interval(first, second) : nullptr;
    }

    void assign(Variable x, Variable y) override {
        unwrapOctagon<N1>(first_)->assign(x, y);
        unwrapOctagon<N2>(second_)->assign(x, y);
    }

    void assign(Variable x, Ptr i) override {
        auto* dint = unwrapDInterval(i);
        unwrapOctagon<N1>(first_)->assign(x, dint->first());
        unwrapOctagon<N2>(second_)->assign(x, dint->second());
    }

    void applyTo(llvm::ArithType op, Variable x, Variable y, Variable z) override {
        unwrapOctagon<N1>(first_)->applyTo(op, x, y, z);
        unwrapOctagon<N2>(second_)->applyTo(op, x, y, z);
    }

    void applyTo(llvm::ArithType op, Variable x, const DNumber& y, Variable z) {
        unwrapOctagon<N1>(first_)->applyTo(op, x, y.first, z);
        unwrapOctagon<N2>(second_)->applyTo(op, x, y.second, z);
    }

    void applyTo(llvm::ArithType op, Variable x, Variable y, const DNumber& z) {
        unwrapOctagon<N1>(first_)->applyTo(op, x, y, z.first);
        unwrapOctagon<N2>(second_)->applyTo(op, x, y, z.second);
    }

    void applyTo(llvm::ArithType op, Variable x, const DNumber& y, const DNumber& z) {
        unwrapOctagon<N1>(first_)->applyTo(op, x, y.first, z.first);
        unwrapOctagon<N2>(second_)->applyTo(op, x, y.second, z.second);
    }

    Ptr applyTo(llvm::ConditionType op, Variable x, Variable y) override {
        auto&& first = unwrapOctagon<N1>(first_)->applyTo(op, x, y);
        auto&& second = unwrapOctagon<N2>(second_)->applyTo(op, x, y);
        return first->join(second);
    }

    Ptr applyTo(llvm::ConditionType op, const DNumber& x, Variable y) {
        auto&& first = unwrapOctagon<N1>(first_)->applyTo(op, x.first, y);
        auto&& second = unwrapOctagon<N2>(second_)->applyTo(op, x.second, y);
        return first->join(second);
    }

    Ptr applyTo(llvm::ConditionType op, Variable x, const DNumber& y) {
        auto&& first = unwrapOctagon<N1>(first_)->applyTo(op, x, y.first);
        auto&& second = unwrapOctagon<N2>(second_)->applyTo(op, x, y.second);
        return first->join(second);
    }

    Ptr applyTo(llvm::ConditionType op, const DNumber& x, const DNumber& y) {
        auto&& first = unwrapOctagon<N1>(first_)->applyTo(op, x.first, y.first);
        auto&& second = unwrapOctagon<N2>(second_)->applyTo(op, x.second, y.second);
        return first->join(second);
    }

    size_t hashCode() const override {
        return class_tag(*this);
    }

    std::string toString() const override {
        std::stringstream ss;
        ss << "{" << std::endl << first_->toString() << ";" << std::endl << second_->toString() << std::endl << "}";
        return ss.str();
    }
};

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"

#endif //BOREALIS_DOUBLEOCTAGON_HPP
