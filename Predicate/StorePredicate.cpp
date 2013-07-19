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
            Predicate(type_id(*this), type),
            lhv(lhv),
            rhv(rhv) {
    this->asString = "*" + this->lhv->getName() + "=" + this->rhv->getName();
}

bool StorePredicate::equals(const Predicate* other) const {
    if (const Self* o = llvm::dyn_cast_or_null<Self>(other)) {
        return *this->lhv == *o->lhv &&
                *this->rhv == *o->rhv;
    } else return false;
}

size_t StorePredicate::hashCode() const {
    return util::hash::defaultHasher()(type, lhv, rhv);
}

} /* namespace borealis */
