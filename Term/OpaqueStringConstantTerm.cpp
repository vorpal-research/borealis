/*
 * OpaqueConstantTerm.cpp
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#include "Term/OpaqueStringConstantTerm.h"

namespace borealis {

OpaqueStringConstantTerm::OpaqueStringConstantTerm(Type::Ptr type, const std::string& value):
    Term(
        class_tag(*this),
        type,
        value
    ), value(value) {};

const std::string& OpaqueStringConstantTerm::getValue() const {
    return value;
}

} // namespace borealis
