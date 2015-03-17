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
        Term::Ptr numElems,
        Term::Ptr origNumElems,
        const Locus& loc,
        PredicateType type) :
            Predicate(class_tag(*this), type, loc) {
    asString = lhv->getName() + "=malloc(" +
        numElems->getName() + "," +
        origNumElems->getName() +
    ")";
    ops = { lhv, numElems, origNumElems };
}

Term::Ptr MallocPredicate::getLhv() const {
    return ops[0];
}

Term::Ptr MallocPredicate::getNumElems() const {
    return ops[1];
}

Term::Ptr MallocPredicate::getOrigNumElems() const {
    return ops[2];
}

} /* namespace borealis */
