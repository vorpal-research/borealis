/*
 * ICmpPredicate.cpp
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#include "ICmpPredicate.h"

namespace borealis {

using llvm::ConditionType;
using llvm::conditionString;
using llvm::conditionType;

using util::sayonara;

ICmpPredicate::ICmpPredicate(
        Term::Ptr lhv,
        Term::Ptr op1,
        Term::Ptr op2,
        int cond) : Predicate(type_id(*this)),
            lhv(std::move(lhv)),
            op1(std::move(op1)),
            op2(std::move(op2)),
            cond(cond),
            _cond(conditionString(cond)) {

    this->asString =
            this->lhv->getName() + "=" + this->op1->getName() + _cond + this->op2->getName();
}

ICmpPredicate::ICmpPredicate(
        Term::Ptr lhv,
        Term::Ptr op1,
        Term::Ptr op2,
        int cond,
        SlotTracker* /* st */) : Predicate(type_id(*this)),
            lhv(std::move(lhv)),
            op1(std::move(op1)),
            op2(std::move(op2)),
            cond(cond),
            _cond(conditionString(cond)) {

    this->asString =
            this->lhv->getName() + "=" + this->op1->getName() + _cond + this->op2->getName();
}

Predicate::Key ICmpPredicate::getKey() const {
    return std::make_pair(borealis::type_id(*this), lhv->getId());
}

z3::expr ICmpPredicate::toZ3(Z3ExprFactory& z3ef, ExecutionContext*) const {
    TRACE_FUNC;

    z3::expr l = z3ef.getExprForTerm(*lhv);
    z3::expr o1 = z3ef.getExprForTerm(*op1);
    z3::expr o2 = z3ef.getExprForTerm(*op2);

    ConditionType ct = conditionType(cond);
    switch(ct) {
    case ConditionType::EQ: return l == (o1 == o2);
    case ConditionType::NEQ: return l == (o1 != o2);
    case ConditionType::LT: return l == (o1 < o2);
    case ConditionType::LTE: return l == (o1 <= o2);
    case ConditionType::GT: return l == (o1 > o2);
    case ConditionType::GTE: return l == (o1 >= o2);
    case ConditionType::TRUE: return l == z3ef.getBoolConst(true);
    case ConditionType::FALSE: return l == z3ef.getBoolConst(false);
    case ConditionType::UNKNOWN: return sayonara<z3::expr>(__FILE__, __LINE__, __PRETTY_FUNCTION__,
            "Unknown condition type in Z3 conversion");
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
    size_t hash = 3;
    hash = 17 * hash + lhv->hashCode();
    hash = 17 * hash + op1->hashCode();
    hash = 17 * hash + op2->hashCode();
    hash = 17 * hash + cond;
    return hash;
}

} /* namespace borealis */
