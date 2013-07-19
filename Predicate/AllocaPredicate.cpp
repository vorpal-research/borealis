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
        PredicateType type) :
            Predicate(type_id(*this), type),
            lhv(lhv),
            numElements(numElements) {
    this->asString = this->lhv->getName() + "=alloca(" + this->numElements->getName() + ")";
}

bool AllocaPredicate::equals(const Predicate* other) const {
    if (const Self* o = llvm::dyn_cast_or_null<Self>(other)) {
        return *this->lhv == *o->lhv &&
                *this->numElements == *o->numElements;
    } else return false;
}

size_t AllocaPredicate::hashCode() const {
    return util::hash::defaultHasher()(type, lhv, numElements);
}

} /* namespace borealis */
