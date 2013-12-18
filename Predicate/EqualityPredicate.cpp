/*
 * EqualityPredicate.cpp
 *
 *  Created on: Oct 26, 2012
 *      Author: ice-phoenix
 */

#include "Predicate/EqualityPredicate.h"

namespace borealis {

EqualityPredicate::EqualityPredicate(
        Term::Ptr lhv,
        Term::Ptr rhv,
        const Locus& loc,
        PredicateType type) :
            Predicate(class_tag(*this), type, loc),
            lhv(lhv),
            rhv(rhv) {
    asString = lhv->getName() + "=" + rhv->getName();
}

bool EqualityPredicate::equals(const Predicate* other) const {
    if (const Self* o = llvm::dyn_cast_or_null<Self>(other)) {
        return Predicate::equals(other) &&
                *lhv == *o->lhv &&
                *rhv == *o->rhv;
    } else return false;
}

size_t EqualityPredicate::hashCode() const {
    return util::hash::defaultHasher()(Predicate::hashCode(), lhv, rhv);
}

} /* namespace borealis */
