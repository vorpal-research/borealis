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
        Term::Ptr origNumElements,
        const Locus& loc,
        PredicateType type) :
            Predicate(class_tag(*this), type, loc),
            lhv(lhv),
            numElements(numElements),
            origNumElements(origNumElements) {
    asString = lhv->getName() + "=alloca(" +
        numElements->getName() + "," +
        origNumElements->getName() +
    ")";
}

bool AllocaPredicate::equals(const Predicate* other) const {
    if (const Self* o = llvm::dyn_cast_or_null<Self>(other)) {
        return Predicate::equals(other) &&
                *lhv == *o->lhv &&
                *numElements == *o->numElements &&
                *origNumElements == *o->origNumElements;
    } else return false;
}

size_t AllocaPredicate::hashCode() const {
    return util::hash::defaultHasher()(Predicate::hashCode(), lhv, numElements, origNumElements);
}

} /* namespace borealis */
