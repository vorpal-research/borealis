/*
 * EqualityPredicate.cpp
 *
 *  Created on: Oct 26, 2012
 *      Author: ice-phoenix
 */

#include "EqualityPredicate.h"

namespace borealis {

EqualityPredicate::EqualityPredicate(
        PredicateType type,
        Term::Ptr lhv,
        Term::Ptr rhv) :
            EqualityPredicate(lhv, rhv, nullptr, type) {}

EqualityPredicate::EqualityPredicate(
        Term::Ptr lhv,
        Term::Ptr rhv,
        SlotTracker* /* st */,
        PredicateType type) :
            Predicate(type_id(*this), type),
            lhv(lhv),
            rhv(rhv) {
    this->asString = this->lhv->getName() + "=" + this->rhv->getName();
}

logic::Bool EqualityPredicate::toZ3(Z3ExprFactory& z3ef, ExecutionContext*) const {
    TRACE_FUNC;

    auto l = z3ef.getExprForTerm(*lhv);
    auto r = z3ef.getExprForTerm(*rhv);
    return l == r;
}

bool EqualityPredicate::equals(const Predicate* other) const {
    if (other == nullptr) return false;
    if (this == other) return true;
    if (const EqualityPredicate* o = llvm::dyn_cast<EqualityPredicate>(other)) {
        return *this->lhv == *o->lhv &&
                *this->rhv == *o->rhv;
    } else {
        return false;
    }
}

size_t EqualityPredicate::hashCode() const {
    size_t hash = 3;
    hash = 17 * hash + lhv->hashCode();
    hash = 17 * hash + rhv->hashCode();
    return hash;
}

} /* namespace borealis */
