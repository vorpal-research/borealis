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

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
class LinearConstraint {
public:

    using LinearExpr = LinearExpression<Number, Variable, VarHash, VarEquals>;
    using CasterT = typename LinearExpr::CasterT;
    using VariableSet = typename LinearExpr::VariableSet;

    enum Kind {
        EQUALITY, // ==
        INEQUALITY, // !=
        COMPARISON // <=
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

    static LinearConstraint tautology(const CasterT* caster) {
        return LinearConstraint(LinearExpr(caster), EQUALITY);
    }

    static LinearConstraint contradiction(const CasterT* caster) {
        return LinearConstraint(LinearExpr(caster), INEQUALITY);
    }

    bool isTautology() const {
        if (!this->expr_.isConstant()) {
            return false;
        }

        switch (this->kind_) {
            case EQUALITY: return this->expr_.constant() == (*expr_.caster())(0);
            case INEQUALITY: return this->expr_.constant() != (*expr_.caster())(0);
            case COMPARISON: return this->expr_.constant() <= (*expr_.caster())(0);
            default: UNREACHABLE("Unexpected kind");
        }
    }

    bool isContradiction() const {
        if (not this->expr_.isConstant()) {
            return false;
        }

        switch (this->kind_) {
            case EQUALITY: return this->expr_.constant() != (*expr_.caster())(0);
            case INEQUALITY: return this->expr_.constant() == (*expr_.caster())(0);
            case COMPARISON: return this->expr_.constant() > (*expr_.caster())(0);
            default: UNREACHABLE("Unexpected kind");
        }
    }

    bool isEquality() const { return this->kind_ == EQUALITY; }
    bool isInequality() const { return this->kind_ == INEQUALITY; }
    bool isComparison() const { return this->kind_ == COMPARISON; }

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

    std::string toString() const {
        std::stringstream ss;
        ss << expr_;
        switch (kind_) {
            case EQUALITY: ss << " == "; break;
            case INEQUALITY: ss << " != "; break;
            case COMPARISON: ss << " <= "; break;
            default: UNREACHABLE("unexpected kind");
        }
        ss << (*expr_.caster())(0);
        return ss.str();
    }
};

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
std::ostream& operator<<(std::ostream& s, const LinearConstraint<Number, Variable, VarHash, VarEquals>& cst) {
    s << cst.toString();
    return s;
}

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const LinearConstraint<Number, Variable, VarHash, VarEquals>& cst) {
    s << cst.toString();
    return s;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
inline LinearConstraint<Number, Variable, VarHash, VarEquals> operator==(
        LinearExpression<Number, Variable, VarHash, VarEquals> e, const Number& n) {
    using ConstT = LinearConstraint<Number, Variable, VarHash, VarEquals>;
    return ConstT(std::move(e) - n, ConstT::EQUALITY);
}

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
inline LinearConstraint<Number, Variable, VarHash, VarEquals> operator==(
        LinearExpression<Number, Variable, VarHash, VarEquals> e, int n) {
    using ConstT = LinearConstraint<Number, Variable, VarHash, VarEquals>;
    return ConstT(std::move(e) - n, ConstT::EQUALITY);
}

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
inline LinearConstraint<Number, Variable, VarHash, VarEquals> operator==(
        LinearExpression<Number, Variable, VarHash, VarEquals> x,
        const LinearExpression<Number, Variable, VarHash, VarEquals>& y) {
    using ConstT = LinearConstraint<Number, Variable, VarHash, VarEquals>;
    return ConstT(std::move(x) - y, ConstT::EQUALITY);
}

///////////////////////////////////////////////////////////////////////////////

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
inline LinearConstraint<Number, Variable, VarHash, VarEquals> operator!=(
        LinearExpression<Number, Variable, VarHash, VarEquals> e, const Number& n) {
    using ConstT = LinearConstraint<Number, Variable, VarHash, VarEquals>;
    return ConstT(std::move(e) - n, ConstT::INEQUALITY);
}

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
inline LinearConstraint<Number, Variable, VarHash, VarEquals> operator!=(
        LinearExpression<Number, Variable, VarHash, VarEquals> e, int n) {
    using ConstT = LinearConstraint<Number, Variable, VarHash, VarEquals>;
    return ConstT(std::move(e) - n, ConstT::INEQUALITY);
}

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
inline LinearConstraint<Number, Variable, VarHash, VarEquals> operator!=(
        LinearExpression<Number, Variable, VarHash, VarEquals> x,
        const LinearExpression<Number, Variable, VarHash, VarEquals>& y) {
    using ConstT = LinearConstraint<Number, Variable, VarHash, VarEquals>;
    return ConstT(std::move(x) - y, ConstT::INEQUALITY);
}

///////////////////////////////////////////////////////////////////////////////

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
inline LinearConstraint<Number, Variable, VarHash, VarEquals> operator<=(
        LinearExpression<Number, Variable, VarHash, VarEquals> e, const Number& n) {
    using ConstT = LinearConstraint<Number, Variable, VarHash, VarEquals>;
    return ConstT(std::move(e) - n, ConstT::COMPARISON);
}

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
inline LinearConstraint<Number, Variable, VarHash, VarEquals> operator<=(
        LinearExpression<Number, Variable, VarHash, VarEquals> e, int n) {
    using ConstT = LinearConstraint<Number, Variable, VarHash, VarEquals>;
    return ConstT(std::move(e) - n, ConstT::COMPARISON);
}

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
inline LinearConstraint<Number, Variable, VarHash, VarEquals> operator<=(
        LinearExpression<Number, Variable, VarHash, VarEquals> x,
        const LinearExpression<Number, Variable, VarHash, VarEquals>& y) {
    using ConstT = LinearConstraint<Number, Variable, VarHash, VarEquals>;
    return ConstT(std::move(x) - y, ConstT::COMPARISON);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
class LinearConstraintSystem {
public:

    using LinearConstraintT = LinearConstraint<Number, Variable, VarHash, VarEquals>;
    using CasterT = typename LinearConstraintT::CasterT;
    using LinearExpr = typename LinearConstraintT::LinearExpr;
    using VariableSet = typename LinearConstraintT::VariableSet;
    using Constraints = std::vector<LinearConstraintT>;

private:

    Constraints constraints_;

private:

    static LinearConstraintSystem makeConditionSystem(llvm::ConditionType op, LinearExpr x, LinearExpr y) {
        LinearConstraintSystem system;

        auto* caster = x.caster();
        auto&& num = LAM(a, (*caster)(a));
        switch (op) {
            case llvm::ConditionType::EQ:
                // x == y
                system.add(x == y);
                break;
            case llvm::ConditionType::NEQ:
                // x != y
                system.add(x != y);
                break;
            case llvm::ConditionType::GT:
            case llvm::ConditionType::UGT:
                // x > y; -x < -y; -x + 1 <= -y
                system.add((-x + num(1)) <= -y);
                break;
            case llvm::ConditionType::GE:
            case llvm::ConditionType::UGE:
                // x >= y; -x <= -y
                system.add(-x <= -y);
                break;
            case llvm::ConditionType::LT:
            case llvm::ConditionType::ULT:
                // x < y; x + 1 <= y
                system.add((x + num(1)) <= y);
                break;
            case llvm::ConditionType::LE:
            case llvm::ConditionType::ULE:
                // x <= y
                system.add(x <= y);
                break;
            case llvm::ConditionType::TRUE:break;
            case llvm::ConditionType::FALSE:break;
            case llvm::ConditionType::UNKNOWN:break;
        }

        return system;
    }

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
            system.add(LinearConstraintT::contradiction(i.caster()));
        } else {
            auto* caster = i.caster();
            LinearExpr expr(caster, v);

            auto& lb = i.lb();
            auto& ub = i.ub();
            if (i.isConstant()) {
                system.add(expr == i.asConstant());
            } else {
                if (lb.isFinite()) {
                    system.add(-expr <= -lb.number());
                } else {
                    ASSERTC(lb.isMinusInfinity());
                    system.add(-expr <= caster->minValue());
                }

                if (ub.isFinite()) {
                    system.add(expr <= ub.number());
                } else {
                    ASSERTC(ub.isPlusInfinity());
                    system.add(expr <= caster->maxValue());
                }
            }
        }
        return system;
    }

    static LinearConstraintSystem withinInterval(Variable v, const CasterT* caster, const std::pair<Number, Number>& i) {
        LinearConstraintSystem system;
        LinearExpr expr(caster, v);

        auto& lb = i.first;
        auto& ub = i.second;
        if (lb > ub) {
            return LinearConstraintSystem(LinearConstraintT::contradiction(caster));
        } else if (lb == ub) {
            system.add(expr == LinearExpr(caster, lb));
        } else {
            system.add(-expr <= LinearExpr(caster, -lb));
            system.add(expr <= LinearExpr(caster, ub));
        }
        return system;
    }

    static LinearConstraintSystem makeCondition(llvm::ConditionType op, const CasterT* caster, Variable x, Variable y) {
        return makeConditionSystem(op, LinearExpr(caster, x), LinearExpr(caster, y));
    }

    static LinearConstraintSystem makeCondition(llvm::ConditionType op, const CasterT* caster, const Number& x, Variable y) {
        return makeConditionSystem(op, LinearExpr(caster, x), LinearExpr(caster, y));
    }

    static LinearConstraintSystem makeCondition(llvm::ConditionType op, const CasterT* caster, Variable x, const Number& y) {
        return makeConditionSystem(op, LinearExpr(caster, x), LinearExpr(caster, y));
    }

    static LinearConstraintSystem makeCondition(llvm::ConditionType op, const CasterT* caster, const Number& x, const Number& y) {
        return makeConditionSystem(op, LinearExpr(caster, x), LinearExpr(caster, y));
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

    std::string toString() const {
        std::stringstream ss;
        for (auto&& it : constraints_)
            ss << it << std::endl;
        return ss.str();
    }
};

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"

#endif //BOREALIS_LINEARCONSTRAINT_HPP
