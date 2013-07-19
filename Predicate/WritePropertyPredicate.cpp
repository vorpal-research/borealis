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
            Predicate(type_id(*this), type),
            propName(propName),
            lhv(lhv),
            rhv(rhv) {
    this->asString = "write(" +
            this->propName->getName() + "," +
            this->lhv->getName() + "," +
            this->rhv->getName() +
        ")";
}

bool WritePropertyPredicate::equals(const Predicate* other) const {
    if (const Self* o = llvm::dyn_cast_or_null<Self>(other)) {
        return *this->propName == *o->propName &&
                *this->lhv == *o->lhv &&
                *this->rhv == *o->rhv;
    } else return false;
}

size_t WritePropertyPredicate::hashCode() const {
    return util::hash::defaultHasher()(type, propName, lhv, rhv);
}

} /* namespace borealis */
