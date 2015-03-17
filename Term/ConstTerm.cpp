/*
 * ConstTerm.cpp
 *
 *  Created on: Nov 16, 2012
 *      Author: ice-phoenix
 */

#include "Term/ConstTerm.h"

namespace borealis {

ConstTerm::ConstTerm(Type::Ptr type, const std::string& name):
    Term(
        class_tag(*this),
        type,
        name
    ) {};

} // namespace borealis
