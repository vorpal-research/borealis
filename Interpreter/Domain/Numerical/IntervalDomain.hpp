//
// Created by abdullin on 1/28/19.
//

#ifndef BOREALIS_INTERVALDOMAIN_HPP
#define BOREALIS_INTERVALDOMAIN_HPP

#include "Interpreter/Domain/AbstractDomain.hpp"
#include "Interpreter/Domain/AbstractFactory.hpp"

#include "Bound.hpp"
#include "NumericalDomain.hpp"
#include "SeparateDomain.hpp"

#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {

class AbstractFactory;

template <typename Number, typename Variable>
class IntervalDomain : public NumericalDomain<Number, Variable> {
public:
    using Self = IntervalDomain<Number, Variable>;
    using Ptr = AbstractDomain::Ptr;
    using ConstPtr = AbstractDomain::ConstPtr;

    using BoundT = Bound<Number>;
    using IntervalT = Interval<Number>;
    using IntervalPtr = typename IntervalT::Ptr;
    using EnvT = typename SeparateDomain<Number, Variable, Interval<Number>>::Ptr;

private:

    EnvT env_;

private:

    explicit IntervalDomain(EnvT env) : AbstractDomain(class_tag(*this)), env_(std::move(env)) {}

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

public:

    IntervalDomain() : IntervalDomain(EnvT::top()) {}
    IntervalDomain(const IntervalDomain&) = default;
    IntervalDomain(IntervalDomain&&) noexcept = default;
    IntervalDomain& operator=(const IntervalDomain&) = default;
    IntervalDomain& operator=(IntervalDomain&&) noexcept = default;
    ~IntervalDomain() override = default;

    Ptr clone() const override {
        return std::make_shared<Self>(*this);
    }

    static bool classof (const Self*) {
        return true;
    }

    static bool classof(const AbstractDomain* other) {
        return other->getClassTag() == class_tag<Self>();
    }

    static Ptr top() { return std::make_shared(EnvT::top()); }
    static Ptr bottom() { return std::make_shared(EnvT::bottom()); }

    bool isTop() const override { return this->env_->isTop(); }
    bool isBottom() const override { return this->env_->isBottom(); }
    void setTop() override { this->env_->setTop(); }
    void setBottom() override { this->env_->setBottom(); }

    bool leq(ConstPtr other) const override { return this->env_->leq(unwrap(other)->env_); }
    bool equals(ConstPtr other) const override { return this->env_->equals(unwrap(other)->env_); }

    void joinWith(ConstPtr other) override { this->env_->joinWith(unwrap(other)->env_); }
    void meetWith(ConstPtr other) override { this->env_->meetWith(unwrap(other)->env_); }
    void widenWith(ConstPtr other) override { this->env_->widenWith(unwrap(other)->env_); }

    IntervalPtr get(Variable x) const override { return this->env_->get(x); }
    void set(Variable x, IntervalPtr value) { return this->env_->set(x, value); }
    void forget(Variable x) { return this->env_->forget(x); }

    IntervalPtr toInterval(Variable x) const override { return this->get(x); }

    void assign(Variable x, int n) override { this->set(x, std::make_shared<IntervalT>(n)); }
    void assign(Variable x, Number& n) override { this->set(x, std::make_shared<IntervalT>(n)); }
    void assign(Variable x, Variable y) override { this->set(x, this->get(y)); }
    void assign(Variable x, IntervalPtr i) override { this->set(x, i); }

    void apply(llvm::ArithType op, Variable x, Variable y, Variable z) override { return this->env_->apply(op, x, y, z); }

    Ptr apply(llvm::ConditionType op, Variable x, Variable y) override {
        auto&& makeTop = []() { return AbstractFactory::get()->getBool(AbstractFactory::TOP); };
        auto&& makeBool = [](bool b) { return AbstractFactory::get()->getBool(b); };

        auto& lhv = this->get(x);
        auto& rhv = this->get(y);

        if (lhv->isTop() || rhv->isTop()) {
            return makeTop();
        } else if (lhv.isBottom() || rhv.isBottom()) {
            return AbstractFactory::get()->getBool(AbstractFactory::BOTTOM);
        }

        switch (op) {
            case llvm::ConditionType::TRUE:
                return makeBool(true);
            case llvm::ConditionType::FALSE:
                return makeBool(false);
            case llvm::ConditionType::UNKNOWN:
                return makeTop();
            case llvm::ConditionType::EQ:
                if (lhv->isConstant() && rhv->isConstant()) {
                    return makeBool(lhv->asConstant() == rhv->asConstant());
                } else if (lhv->intersects(rhv)) {
                    return makeTop();
                } else {
                    return makeBool(false);
                }
            case llvm::ConditionType::NEQ:
                if (lhv->isConstant() && rhv->isConstant()) {
                    return makeBool(lhv->asConstant() != rhv->asConstant());
                } else if (lhv->intersects(rhv)) {
                    return makeTop();
                } else {
                    return makeBool(true);
                }
            case llvm::ConditionType::LT:
                if (lhv->intersects(rhv)) {
                    return makeTop();
                } else {
                    return makeBool(lhv->ub() < rhv->lb());
                }
            case llvm::ConditionType::LE:
                if (lhv->intersects(rhv)) {
                    return makeTop();
                } else {
                    return makeBool(lhv->ub() <= rhv->lb());
                }
            case llvm::ConditionType::GT:
                if (lhv->intersects(rhv)) {
                    return makeTop();
                } else {
                    return makeBool(rhv->ub() < lhv->lb());
                }
            case llvm::ConditionType::GE:
                if (lhv->intersects(rhv)) {
                    return makeTop();
                } else {
                    return makeBool(rhv->ub() <= lhv->lb());
                }
            default:
                UNREACHABLE("Unknown cmp operation");
        }
    }

};

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"

#endif //BOREALIS_INTERVALDOMAIN_HPP
