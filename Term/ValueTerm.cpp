/*
 * ValueTerm.cpp
 *
 *  Created on: Nov 16, 2012
 *      Author: ice-phoenix
 */

#include "Term/ValueTerm.h"

namespace borealis {

ValueTerm::ValueTerm(Type::Ptr type, const std::string& name, bool global) :
    Term(
        class_tag(*this),
        type,
        name + (global ? ".global" : "")
    ), global(global) {};

bool ValueTerm::isGlobal() const {
    return global;
}

} // namespace borealis
