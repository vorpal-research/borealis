/*
 * StorePredicate.cpp
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#include "Predicate/StorePredicate.h"

namespace borealis {

StorePredicate::StorePredicate(
        Term::Ptr lhv,
        Term::Ptr rhv,
        PredicateType type) :
            Predicate(class_tag(*this), type),
            lhv(lhv),
            rhv(rhv) {
    asString = "*" + lhv->getName() + "=" + rhv->getName();
}

bool StorePredicate::equals(const Predicate* other) const {
    if (const Self* o = llvm::dyn_cast_or_null<Self>(other)) {
        return Predicate::equals(other) &&
                *lhv == *o->lhv &&
                *rhv == *o->rhv;
    } else return false;
}

size_t StorePredicate::hashCode() const {
    return util::hash::defaultHasher()(Predicate::hashCode(), lhv, rhv);
}

} /* namespace borealis */
