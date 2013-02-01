/*
 * ICmpPredicate.cpp
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#include "Predicate/ICmpPredicate.h"
#include "Util/macros.h"

namespace borealis {

using llvm::conditionString;
using llvm::conditionType;

ICmpPredicate::ICmpPredicate(
        PredicateType type,
        Term::Ptr lhv,
        Term::Ptr op1,
        Term::Ptr op2,
        int cond) :
            ICmpPredicate(lhv, op1, op2, cond, nullptr, type) {}

ICmpPredicate::ICmpPredicate(
        Term::Ptr lhv,
        Term::Ptr op1,
        Term::Ptr op2,
        int cond,
        SlotTracker* /* st */,
        PredicateType type) :
            Predicate(type_id(*this), type),
            lhv(lhv),
            op1(op1),
            op2(op2),
            cond(cond),
            _cond(conditionString(cond)) {

    this->asString =
            this->lhv->getName() + "=" + this->op1->getName() + _cond + this->op2->getName();
}

logic::Bool ICmpPredicate::toZ3(Z3ExprFactory& z3ef, ExecutionContext*) const {
    TRACE_FUNC;

    using llvm::ConditionType;

    auto le = z3ef.getExprForTerm(*lhv).toBool();
    if (le.empty()) {
        BYE_BYE(logic::Bool, "Encountered cmp with non-Bool condition");
    }

    auto l = le.getUnsafe();

    auto o1 = z3ef.getExprForTerm(*op1);
    auto o2 = z3ef.getExprForTerm(*op2);

    ConditionType ct = conditionType(cond);
    // these cases assume nothing about operands
    switch(ct) {
        case ConditionType::EQ: return l == (o1 == o2);
        case ConditionType::NEQ: return l == (o1 != o2);
        case ConditionType::TRUE: return l == z3ef.getTrue();
        case ConditionType::FALSE: return l == z3ef.getFalse();
        default: break;
    }

    // these cases assume operands are comparable
    if(!o1.isComparable() || !o2.isComparable()) {
        BYE_BYE(logic::Bool, "Encountered cmp with non-comparable operands");
    }

    auto co1 = o1.toComparable().getUnsafe();
    auto co2 = o2.toComparable().getUnsafe();
    switch(ct) {
        case ConditionType::LT: return l == (co1 < co2);
        case ConditionType::LTE: return l == (co1 <= co2);
        case ConditionType::GT: return l == (co1 > co2);
        case ConditionType::GTE: return l == (co1 >= co2);
        case ConditionType::UNKNOWN:
            BYE_BYE(logic::Bool, "Unknown condition type in Z3 conversion");
        default:
            BYE_BYE(logic::Bool, "Unreachable");
    }
}

bool ICmpPredicate::equals(const Predicate* other) const {
    if (other == nullptr) return false;
    if (this == other) return true;
    if (const ICmpPredicate* o = llvm::dyn_cast<ICmpPredicate>(other)) {
        return *this->lhv == *o->lhv &&
                *this->op1 == *o->op1 &&
                *this->op2 == *o->op2 &&
                this->cond == o->cond;
    } else {
        return false;
    }
}

size_t ICmpPredicate::hashCode() const {
    return util::hash::hasher<3, 17>()(lhv, op1, op2, cond);
}

} /* namespace borealis */

#include "Util/unmacros.h"
