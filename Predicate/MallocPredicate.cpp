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
        PredicateType type) :
            Predicate(type_id(*this), type),
            lhv(lhv),
            numElements(numElements) {
    this->asString = this->lhv->getName() + "=malloc(" + this->numElements->getName() + ")";
}

bool MallocPredicate::equals(const Predicate* other) const {
    if (const Self* o = llvm::dyn_cast_or_null<Self>(other)) {
        return *this->lhv == *o->lhv;
    } else return false;
}

size_t MallocPredicate::hashCode() const {
    return util::hash::defaultHasher()(type, lhv);
}

} /* namespace borealis */
