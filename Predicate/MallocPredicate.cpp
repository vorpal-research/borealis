/*
 * MallocPredicate.cpp
 *
 *  Created on: Nov 30, 2012
 *      Author: ice-phoenix
 */

#include "Predicate/MallocPredicate.h"

namespace borealis {

MallocPredicate::MallocPredicate(
        Term::Ptr lhv,
        Term::Ptr numElements,
        const Locus& loc,
        PredicateType type) :
            Predicate(class_tag(*this), type, loc),
            lhv(lhv),
            numElements(numElements) {
    asString = lhv->getName() + "=malloc(" + numElements->getName() + ")";
}

bool MallocPredicate::equals(const Predicate* other) const {
    if (const Self* o = llvm::dyn_cast_or_null<Self>(other)) {
        return Predicate::equals(other) &&
                *lhv == *o->lhv &&
                *numElements == *o->numElements;
    } else return false;
}

size_t MallocPredicate::hashCode() const {
    return util::hash::defaultHasher()(Predicate::hashCode(), lhv, numElements);
}

} /* namespace borealis */
