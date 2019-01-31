//
// Created by abdullin on 1/28/19.
//

#ifndef BOREALIS_INTERVALDOMAIN_HPP
#define BOREALIS_INTERVALDOMAIN_HPP

#include "NumericalDomain.hpp"
#include "SeparateDomain.hpp"

#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {

template <typename Number, typename Variable>
class IntervalDomain : public NumericalDomain<Number, Variable, Interval<Number>> {
public:
    using BoolT = Interval<IntNumber<1, false>>;
    using BoundT = Bound<Number>;
    using IntervalT = Interval<Number>;
    using EnvT = SeparateDomain<Number, Variable, Interval<Number>>;

private:

    EnvT env_;

private:

    explicit IntervalDomain(EnvT env) : env_(std::move(env)) {}

public:

    IntervalDomain() : IntervalDomain(EnvT::top()) {}
    IntervalDomain(const IntervalDomain&) = default;
    IntervalDomain(IntervalDomain&&) noexcept = default;
    IntervalDomain& operator=(const IntervalDomain&) = default;
    IntervalDomain& operator=(IntervalDomain&&) noexcept = default;
    ~IntervalDomain() override = default;

    static IntervalDomain top() { return IntervalDomain(EnvT::top()); }
    static IntervalDomain bottom() { return IntervalDomain(EnvT::bottom()); }

    bool isTop() const override { return this->env_.isTop(); }
    bool isBottom() const override { return this->env_.isBottom(); }
    void setTop() override { this->env_.setTop(); }
    void setBottom() override { this->env_.setBottom(); }

    bool leq(const IntervalDomain& other) const override { return this->env_.leq(other.env_); }
    bool equals(const IntervalDomain& other) const override { return this->env_.equals(other.env_); }

    void joinWith(const IntervalDomain& other) override { this->env_.joinWith(other.env_); }
    void meetWith(const IntervalDomain& other) override { this->env_.meetWith(other.env_); }
    void widenWith(const IntervalDomain& other) override { this->env_.widenWith(other.env_); }

    IntervalT& get(Variable x) const { return this->env_.get(x); }
    void set(Variable x, const IntervalT& value) { return this->env_.set(x, value); }
    void forget(Variable x) { return this->env_.forget(x); }

    IntervalT toInterval(Variable x) const override { return this->get(x); }

    void assign(Variable x, int n) override { this->set(x, IntervalT(n)); }
    void assign(Variable x, Number& n) override { this->set(x, IntervalT(n)); }
    void assign(Variable x, Variable y) override { this->set(x, this->get(y)); }
    void assign(Variable x, IntervalT& i) override { this->set(x, i); }

    void apply(BinaryOperator op, Variable x, Variable y, Variable z) override { return this->env_.apply(op, x, y, z); }

    BoolT apply(CmpOperator op, Variable x, Variable y) override {
        auto&& makeBool = [](bool b) { return BoolT((int) b); };

        auto& lhv = this->get(x);
        auto& rhv = this->get(y);

        if (lhv.isTop() || rhv.isTop()) {
            return BoolT::top();
        } else if (lhv.isBottom() || rhv.isBottom()) {
            return BoolT::bottom();
        }

        switch (op) {
            case EQ:
                if (lhv.isConstant() && rhv.isConstant()) {
                    return makeBool(lhv.asConstant() == rhv.asConstant());
                } else if (lhv.intersects(rhv)) {
                    return BoolT::top();
                } else {
                    return makeBool(false);
                }
            case NEQ:
                if (lhv.isConstant() && rhv.isConstant()) {
                    return makeBool(lhv.asConstant() != rhv.asConstant());
                } else if (lhv.intersects(rhv)) {
                    return BoolT::top();
                } else {
                    return makeBool(true);
                }
            case LT:
                if (lhv.intersects(rhv)) {
                    return BoolT::top();
                } else {
                    return makeBool(lhv.ub() < rhv.lb());
                }
            case LE:
                if (lhv.intersects(rhv)) {
                    return BoolT::top();
                } else {
                    return makeBool(lhv.ub() <= rhv.lb());
                }
            case GT:
                if (lhv.intersects(rhv)) {
                    return BoolT::top();
                } else {
                    return makeBool(rhv.ub() < lhv.lb());
                }
            case GE:
                if (lhv.intersects(rhv)) {
                    return BoolT::top();
                } else {
                    return makeBool(rhv.ub() <= lhv.lb());
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
