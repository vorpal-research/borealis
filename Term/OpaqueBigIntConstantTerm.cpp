/*
 * OpaqueConstantTerm.cpp
 *
 *  Created on: Mar 10, 2016
 *      Author: belyaev
 */

#include "Term/OpaqueBigIntConstantTerm.h"

namespace borealis {

OpaqueBigIntConstantTerm::OpaqueBigIntConstantTerm(Type::Ptr type, int64_t value):
    Term(
        class_tag(*this),
        type,
        util::toString(value)
    ) {};

OpaqueBigIntConstantTerm::OpaqueBigIntConstantTerm(Type::Ptr type, const std::string& value):
    Term(
        class_tag(*this),
        type,
        value
    ) {};

const std::string& OpaqueBigIntConstantTerm::getRepresentation() const {
    return getName();
}

} // namespace borealis
