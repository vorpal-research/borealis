/*
 * InequalityPredicate.cpp
 *
 *  Created on: Oct 26, 2012
 *      Author: ice-phoenix
 */

#include "Predicate/InequalityPredicate.h"

namespace borealis {

InequalityPredicate::InequalityPredicate(
        Term::Ptr lhv,
        Term::Ptr rhv,
        const Locus& loc,
        PredicateType type) :
            Predicate(class_tag(*this), type, loc) {
    asString = lhv->getName() + "!=" + rhv->getName();
    ops = { lhv, rhv };
}

Term::Ptr InequalityPredicate::getLhv() const {
    return ops[0];
}

Term::Ptr InequalityPredicate::getRhv() const {
    return ops[1];
}

} /* namespace borealis */
