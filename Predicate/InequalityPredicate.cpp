/*
 * InequalityPredicate.cpp
 *
 *  Created on: Oct 26, 2012
 *      Author: ice-phoenix
 */

#include "Predicate/InequalityPredicate.h"

namespace borealis {

InequalityPredicate::InequalityPredicate(
        PredicateType type,
        Term::Ptr lhv,
        Term::Ptr rhv) :
        InequalityPredicate(lhv, rhv, nullptr, type) {}

InequalityPredicate::InequalityPredicate(
        Term::Ptr lhv,
        Term::Ptr rhv,
        SlotTracker* /* st */,
        PredicateType type) :
            Predicate(type_id(*this), type),
            lhv(lhv),
            rhv(rhv) {
    this->asString = this->lhv->getName() + "=" + this->rhv->getName();
}

logic::Bool InequalityPredicate::toZ3(Z3ExprFactory& z3ef, ExecutionContext*) const {
    TRACE_FUNC;

    auto l = z3ef.getExprForTerm(*lhv);
    auto r = z3ef.getExprForTerm(*rhv);
    return l != r;
}

bool InequalityPredicate::equals(const Predicate* other) const {
    if (other == nullptr) return false;
    if (this == other) return true;
    if (const InequalityPredicate* o = llvm::dyn_cast<InequalityPredicate>(other)) {
        return *this->lhv == *o->lhv &&
                *this->rhv == *o->rhv;
    } else {
        return false;
    }
}

size_t InequalityPredicate::hashCode() const {
    return util::hash::hasher<3, 17>()(lhv, rhv);
}

} /* namespace borealis */
