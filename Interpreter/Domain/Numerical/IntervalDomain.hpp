//
// Created by abdullin on 1/28/19.
//

#ifndef BOREALIS_INTERVALDOMAIN_HPP
#define BOREALIS_INTERVALDOMAIN_HPP

#include "Interpreter/Domain/AbstractFactory.hpp"

#include "Bound.hpp"
#include "Interval.hpp"
#include "NumericalDomain.hpp"
#include "Interpreter/Domain/SeparateDomain.hpp"

#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {

template <typename IntervalT, typename Variable>
class IntervalDomain : public NumericalDomain<Variable> {
public:
    using Self = IntervalDomain<IntervalT, Variable>;
    using Ptr = AbstractDomain::Ptr;
    using ConstPtr = AbstractDomain::ConstPtr;

    using EnvT = SeparateDomain<Variable, IntervalT>;
    using EnvPtr = typename EnvT::Ptr;

protected:

    EnvPtr env_;

private:

    explicit IntervalDomain(EnvPtr env) : NumericalDomain<Variable>(class_tag(*this)), env_(env) {}

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

protected:

    EnvT* unwrapEnv() const {
        auto* envRaw = llvm::dyn_cast<EnvT>(env_.get());
        ASSERTC(envRaw);

        return envRaw;
    }

public:

    IntervalDomain() : IntervalDomain(EnvT::bottom()) {}
    IntervalDomain(const IntervalDomain&) = default;
    IntervalDomain(IntervalDomain&&) = default;
    IntervalDomain& operator=(const IntervalDomain&) = default;
    IntervalDomain& operator=(IntervalDomain&&) = default;
    virtual ~IntervalDomain() = default;

    static bool classof(const Self*) {
        return true;
    }

    static bool classof(const AbstractDomain* other) {
        return other->getClassTag() == class_tag<Self>();
    }

    bool isTop() const override { return this->env_->isTop(); }
    bool isBottom() const override { return this->env_->isBottom(); }
    void setTop() override { this->env_->setTop(); }
    void setBottom() override { this->env_->setBottom(); }

    bool leq(ConstPtr other) const override { return this->env_->leq(unwrap(other)->env_); }
    bool equals(ConstPtr other) const override { return this->env_->equals(unwrap(other)->env_); }

    void joinWith(ConstPtr other) override { this->env_->joinWith(unwrap(other)->env_); }
    void meetWith(ConstPtr other) override { this->env_->meetWith(unwrap(other)->env_); }
    void widenWith(ConstPtr other) override { this->env_->widenWith(unwrap(other)->env_); }

    Ptr join(ConstPtr other) const override {
        auto&& result = this->clone();
        result->joinWith(other);
        return result;
    }

    Ptr meet(ConstPtr other) const override {
        auto&& result = this->clone();
        result->meetWith(other);
        return result;
    }

    Ptr widen(ConstPtr other) const override {
        auto&& result = this->clone();
        result->widenWith(other);
        return result;
    }

    void set(Variable x, Ptr value) { return unwrapEnv()->set(x, value); }
    void forget(Variable x) { return unwrapEnv()->forget(x); }

    Ptr toInterval(Variable x) const override { return this->get(x); }

    void assign(Variable x, Variable y) override { this->set(x, this->get(y)); }
    void assign(Variable x, Ptr i) override { this->set(x, i); }

    void applyTo(llvm::ArithType op, Variable x, Variable y, Variable z) override {
        unwrapEnv()->set(x, this->get(y)->apply(op, this->get(z)));
    }

    Ptr applyTo(llvm::ConditionType op, Variable x, Variable y) override {
        return this->get(x)->apply(op, this->get(y));
    }

    size_t hashCode() const override {
        return this->env_->hashCode();
    }

    std::string toString() const override {
        return env_->toString();
    }

};

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"

#endif //BOREALIS_INTERVALDOMAIN_HPP
