/*
 * OpaqueConstantTerm.cpp
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#include "Term/OpaqueFloatingConstantTerm.h"

namespace borealis {

OpaqueFloatingConstantTerm::OpaqueFloatingConstantTerm(Type::Ptr type, double value):
    Term(
        class_tag(*this),
        type,
        util::toString(value)
    ), value(value) {};

double OpaqueFloatingConstantTerm::getValue() const {
    return value;
}

bool OpaqueFloatingConstantTerm::equals(const Term* other) const {
    if (auto* that = llvm::dyn_cast_or_null<Self>(other)) {
        return Term::equals(other) &&
                std::abs(that->value - value) < EPS;
    } else return false;
}

size_t OpaqueFloatingConstantTerm::hashCode() const {
    return util::hash::defaultHasher()(Term::hashCode(), value);
}

} // namespace borealis
