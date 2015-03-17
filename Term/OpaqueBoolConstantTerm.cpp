/*
 * OpaqueBoolConstantTerm.cpp
 *
 *  Created on: Jan 21, 2013
 *      Author: belyaev
 */

#include "Term/OpaqueBoolConstantTerm.h"

namespace borealis {

OpaqueBoolConstantTerm::OpaqueBoolConstantTerm(Type::Ptr type, bool value):
    Term(
        class_tag(*this),
        type,
        value ? "true" : "false"
    ), value(value) {};

bool OpaqueBoolConstantTerm::getValue() const {
    return value;
}

} // namespace borealis
