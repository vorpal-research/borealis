/*
 * AllocaPredicate.cpp
 *
 *  Created on: Nov 28, 2012
 *      Author: belyaev
 */

#include "Predicate/AllocaPredicate.h"

namespace borealis {

AllocaPredicate::AllocaPredicate(
        Term::Ptr lhv,
        Term::Ptr numElements,
        const Locus& loc,
        PredicateType type) :
            Predicate(class_tag(*this), type, loc),
            lhv(lhv),
            numElements(numElements) {
    asString = lhv->getName() + "=alloca(" + numElements->getName() + ")";
}

bool AllocaPredicate::equals(const Predicate* other) const {
    if (const Self* o = llvm::dyn_cast_or_null<Self>(other)) {
        return Predicate::equals(other) &&
                *lhv == *o->lhv &&
                *numElements == *o->numElements;
    } else return false;
}

size_t AllocaPredicate::hashCode() const {
    return util::hash::defaultHasher()(Predicate::hashCode(), lhv, numElements);
}

} /* namespace borealis */
