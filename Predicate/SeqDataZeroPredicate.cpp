/*
 * SeqDataZeroPredicate.cpp
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#include "Predicate/SeqDataZeroPredicate.h"

namespace borealis {

SeqDataZeroPredicate::SeqDataZeroPredicate(
        Term::Ptr base,
        size_t size,
        const Locus& loc,
        PredicateType type) :
            Predicate(class_tag(*this), type, loc), size(size) {

    asString = base->getName() + "=(0 x " + util::toString(size) + ")";

    ops.insert(ops.end(), base);
}

Term::Ptr SeqDataZeroPredicate::getBase() const {
    return ops[0];
}

} /* namespace borealis */
