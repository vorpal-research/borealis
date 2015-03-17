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
        const Locus& loc,
        PredicateType type) :
            Predicate(class_tag(*this), type, loc) {
    asString = "*" + lhv->getName() + "=" + rhv->getName();
    ops = { lhv, rhv };
}

Term::Ptr StorePredicate::getLhv() const {
    return ops[0];
}

Term::Ptr StorePredicate::getRhv() const {
    return ops[1];
}

} /* namespace borealis */
