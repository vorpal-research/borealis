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
        PredicateType type) :
            Predicate(class_tag(*this), type),
            lhv(lhv),
            rhv(rhv) {
    asString = lhv->getName() + "!=" + rhv->getName();
}

bool InequalityPredicate::equals(const Predicate* other) const {
    if (const Self* o = llvm::dyn_cast_or_null<Self>(other)) {
        return Predicate::equals(other) &&
                *lhv == *o->lhv &&
                *rhv == *o->rhv;
    } else return false;
}

size_t InequalityPredicate::hashCode() const {
    return util::hash::defaultHasher()(Predicate::hashCode(), lhv, rhv);
}

} /* namespace borealis */
