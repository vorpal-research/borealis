/*
 * WritePropertyPredicate.cpp
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#include "Predicate/WritePropertyPredicate.h"

namespace borealis {

WritePropertyPredicate::WritePropertyPredicate(
        Term::Ptr propName,
        Term::Ptr lhv,
        Term::Ptr rhv,
        const Locus& loc,
        PredicateType type) :
            Predicate(class_tag(*this), type, loc) {
    asString = "write(" +
        propName->getName() + "," +
        lhv->getName() + "," +
        rhv->getName() +
    ")";
    ops = { lhv, rhv, propName };
}

Term::Ptr WritePropertyPredicate::getLhv() const {
    return ops[0];
}

Term::Ptr WritePropertyPredicate::getRhv() const {
    return ops[1];
}

Term::Ptr WritePropertyPredicate::getPropertyName() const {
    return ops[2];
}

} /* namespace borealis */
