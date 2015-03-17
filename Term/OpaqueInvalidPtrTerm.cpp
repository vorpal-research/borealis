/*
 * OpaqueInvalidPtrTerm.cpp
 *
 *  Created on: Sep 17, 2013
 *      Author: sam
 */

#include "Term/OpaqueInvalidPtrTerm.h"

namespace borealis {

OpaqueInvalidPtrTerm::OpaqueInvalidPtrTerm(Type::Ptr type):
    Term(
        class_tag(*this),
        type,
        "<invalid>"
    ) {};

} // namespace borealis
