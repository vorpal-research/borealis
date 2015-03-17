/*
 * OpaqueConstantTerm.cpp
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#include "Term/OpaqueIntConstantTerm.h"

namespace borealis {

OpaqueIntConstantTerm::OpaqueIntConstantTerm(Type::Ptr type, long long value):
    Term(
        class_tag(*this),
        type,
        util::toString(value)
    ), value(value) {};

long long OpaqueIntConstantTerm::getValue() const {
    return value;
}

} // namespace borealis
