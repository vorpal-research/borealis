/*
 * WriteBoundPredicate.cpp
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#include "Predicate/WriteBoundPredicate.h"

namespace borealis {

WriteBoundPredicate::WriteBoundPredicate(
        Term::Ptr lhv,
        Term::Ptr rhv,
        const Locus& loc,
        PredicateType type) :
            Predicate(class_tag(*this), type, loc),
            lhv(lhv),
            rhv(rhv) {
    asString = "writeBound(" +
            lhv->getName() + "," +
            rhv->getName() +
        ")";
}

bool WriteBoundPredicate::equals(const Predicate* other) const {
    if (const Self* o = llvm::dyn_cast_or_null<Self>(other)) {
        return Predicate::equals(other) &&
                *lhv == *o->lhv &&
                *rhv == *o->rhv;
    } else return false;
}

size_t WriteBoundPredicate::hashCode() const {
    return util::hash::defaultHasher()(Predicate::hashCode(), lhv, rhv);
}

} /* namespace borealis */
