/*
 * EqualityPredicate.cpp
 *
 *  Created on: Oct 26, 2012
 *      Author: ice-phoenix
 */

#include "Predicate/EqualityPredicate.h"

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

logic::Bool EqualityPredicate::toZ3(Z3ExprFactory& z3ef, ExecutionContext* ctx) const {
    TRACE_FUNC;
    return lhv->toZ3(z3ef, ctx) == rhv->toZ3(z3ef, ctx);
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
    return util::hash::hasher<3, 17>()(lhv, rhv);
}

} /* namespace borealis */
