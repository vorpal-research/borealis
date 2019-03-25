//
// Created by abdullin on 3/25/19.
//

#ifndef BOREALIS_LINEARCONSTRAINT_HPP
#define BOREALIS_LINEARCONSTRAINT_HPP

#include "LinearExpression.hpp"

#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {
namespace apron {

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
class LinearConstraint {
public:

    using LinearExpr = LinearExpression<Number, Variable, VarHash, VarEquals>;
    using CasterT = typename LinearExpr::CasterT;
    using VariableSet = typename LinearExpr::VariableSet;

    enum Kind {
        EQUALITY,
        INEQUALITY,
        COMPARSION
    };

private:

    LinearExpr expr_;
    Kind kind_;

public:

    LinearConstraint() = delete;
    LinearConstraint(LinearExpr expr, Kind kind) : expr_(std::move(expr)), kind_(kind) {}

    LinearConstraint(const LinearConstraint&) = default;
    LinearConstraint(LinearConstraint&&) = default;
    LinearConstraint& operator=(const LinearConstraint&) = default;
    LinearConstraint& operator=(LinearConstraint&&) = default;
    ~LinearConstraint() = default;

    static LinearConstraint tautology() {
        return LinearConstraint(LinearExpr(), EQUALITY);
    }

    static LinearConstraint contradiction() {
        return LinearConstraint(LinearExpr(), INEQUALITY);
    }

    bool isTautology() const {
        if (!this->expr_.isConstant()) {
            return false;
        }

        switch (this->kind_) {
            case EQUALITY: return this->expr_.constant() == 0;
            case INEQUALITY: return this->expr_.constant() != 0;
            case COMPARSION: return this->expr_.constant() <= 0;
            default: UNREACHABLE("Unexpected kind");
        }
    }

    bool isContradiction() const {
        if (not this->expr_.isConstant()) {
            return false;
        }

        switch (this->kind_) {
            case EQUALITY: return this->expr_.constant() != 0;
            case INEQUALITY: return this->expr_.constant() == 0;
            case COMPARSION: return this->expr_.constant() > 0;
            default: UNREACHABLE("Unexpected kind");
        }
    }

    bool isEquality() const { return this->kind_ == EQUALITY; }
    bool isDisequation() const { return this->kind_ == INEQUALITY; }
    bool is_inequality() const { return this->kind_ == COMPARSION; }

    const LinearExpr& expression() const { return this->expr_; }

    Kind kind() const { return this->kind_; }
    Number constant() const { return -this->expr_.constant(); }
    std::size_t numTerms() const { return this->expr_.numTerms(); }
    Number factor(Variable var) const { return this->expr_.factor(var); }

    VariableSet variables() const {
        return this->expr_.variables();
    }

    auto begin() QUICK_RETURN(expr_.begin());
    auto begin() QUICK_CONST_RETURN(expr_.begin());

    auto end() QUICK_RETURN(expr_.end());
    auto end() QUICK_CONST_RETURN(expr_.end());
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
inline LinearConstraint<Number, Variable, VarHash, VarEquals> operator==(
        LinearExpression<Number, Variable, VarHash, VarEquals> e, const Number& n) {
    return LinearConstraint<Number, Variable, VarHash, VarEquals>(
            std::move(e) - n,
            LinearConstraint<Number, Variable, VarHash, VarEquals>::EQUALITY
        );
}

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
inline LinearConstraint<Number, Variable, VarHash, VarEquals> operator==(
        LinearExpression<Number, Variable, VarHash, VarEquals> e, int n) {
    return LinearConstraint<Number, Variable, VarHash, VarEquals>(std::move(e) - n,
                          LinearConstraint<Number, Variable, VarHash, VarEquals>::Equality);
}

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
inline LinearConstraint<Number, Variable, VarHash, VarEquals> operator==(
        LinearExpression<Number, Variable, VarHash, VarEquals> x,
        const LinearExpression<Number, Variable, VarHash, VarEquals>& y) {
    return LinearConstraint<Number, Variable, VarHash, VarEquals>(std::move(x) - y,
                          LinearConstraint<Number, Variable, VarHash, VarEquals>::Equality);
}

///////////////////////////////////////////////////////////////////////////////

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
inline LinearConstraint<Number, Variable, VarHash, VarEquals> operator!=(
        LinearExpression<Number, Variable, VarHash, VarEquals> e, const Number& n) {
    return LinearConstraint<Number, Variable, VarHash, VarEquals>(std::move(e) - n,
                          LinearConstraint<Number, Variable, VarHash, VarEquals>::Disequation);
}

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
inline LinearConstraint<Number, Variable, VarHash, VarEquals> operator!=(
        LinearExpression<Number, Variable, VarHash, VarEquals> e, int n) {
    return LinearConstraint<Number, Variable, VarHash, VarEquals>(std::move(e) - n,
                          LinearConstraint<Number, Variable, VarHash, VarEquals>::Disequation);
}

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
inline LinearConstraint<Number, Variable, VarHash, VarEquals> operator!=(
        LinearExpression<Number, Variable, VarHash, VarEquals> x,
        const LinearExpression<Number, Variable, VarHash, VarEquals>& y) {
    return LinearConstraint<Number, Variable, VarHash, VarEquals>(std::move(x) - y,
                          LinearConstraint<Number, Variable, VarHash, VarEquals>::Disequation);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
class LinearConstraintSystem {
public:

    using LinearConstraintT = LinearConstraint<Number, Variable, VarHash, VarEquals>;
    using LinearExpr = typename LinearConstraintT::LinearExpr;
    using VariableSet = typename LinearConstraintT::VariableSet;
    using Constraints = std::vector<LinearConstraintT>;

private:

    Constraints constraints_;

public:
    LinearConstraintSystem() = default;
    explicit LinearConstraintSystem(LinearConstraintT cst)
            : constraints_{ std::move(cst) } {}
    LinearConstraintSystem(std::initializer_list< LinearConstraintT > csts)
            : constraints_(std::move(csts)) {}

    LinearConstraintSystem(const LinearConstraintSystem&) = default;
    LinearConstraintSystem(LinearConstraintSystem&&) = default;
    LinearConstraintSystem& operator=(const LinearConstraintSystem&) = default;
    LinearConstraintSystem& operator=(LinearConstraintSystem&&) = default;
    ~LinearConstraintSystem() = default;

    static LinearConstraintSystem withinInterval(Variable v, const Interval<Number>& i) {
        LinearConstraintSystem system;
        if (i.isBottom()) {
            system.add(LinearConstraintT::contradiction());
        } else {
            LinearExpr expr(v);

            auto& lb = i.lb();
            auto& ub = i.ub();
            if (i.isConstant()) {
                system.add(expr == i.asConstant());
            } else {
                if (lb.isFinite()) system.add(lb.number() <= expr);
                if (ub.isFinite()) system.add(expr <= ub.number());
            }
        }
        return system;
    }

    bool empty() const { return this->constraints_.empty(); }
    std::size_t size() const { return this->constraints_.size(); }
    void add(LinearConstraintT cst) { this->constraints_.emplace_back(std::move(cst)); }

    void add(const LinearConstraintSystem& csts) {
        constraints_.reserve(constraints_.size() + csts.size());
        constraints_.insert(constraints_.end(), csts.begin(), csts.end());
    }

    void add(LinearConstraintSystem&& csts) {
        constraints_.reserve(constraints_.size() + csts.size());
        constraints_.insert(constraints_.end(),
                           std::make_move_iterator(csts.begin()),
                           std::make_move_iterator(csts.end()));
    }

    auto begin() QUICK_RETURN(constraints_.begin());
    auto begin() QUICK_CONST_RETURN(constraints_.begin());

    auto end() QUICK_RETURN(constraints_.end());
    auto end() QUICK_CONST_RETURN(constraints_.end());

    VariableSet variables() const {
        VariableSet vars;
        for (auto&& cst : this->constraints_) {
            for (auto&& term : cst) {
                vars.insert(term.first);
            }
        }
        return vars;
    }
};

} // namespace apron
} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"

#endif //BOREALIS_LINEARCONSTRAINT_HPP
