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
        PredicateType type) :
            Predicate(class_tag(*this), type),
            propName(propName),
            lhv(lhv),
            rhv(rhv) {
    asString = "write(" +
            propName->getName() + "," +
            lhv->getName() + "," +
            rhv->getName() +
        ")";
}

bool WritePropertyPredicate::equals(const Predicate* other) const {
    if (const Self* o = llvm::dyn_cast_or_null<Self>(other)) {
        return Predicate::equals(other) &&
                *propName == *o->propName &&
                *lhv == *o->lhv &&
                *rhv == *o->rhv;
    } else return false;
}

size_t WritePropertyPredicate::hashCode() const {
    return util::hash::defaultHasher()(Predicate::hashCode(), propName, lhv, rhv);
}

} /* namespace borealis */
