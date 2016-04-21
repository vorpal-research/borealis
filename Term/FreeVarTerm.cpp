/*
 * FreeVarTerm.cpp
 *
 *  Created on: Nov 16, 2012
 *      Author: ice-phoenix
 */

#include "Term/FreeVarTerm.h"

namespace borealis {

FreeVarTerm::FreeVarTerm(Type::Ptr type, const std::string& vname) :
    Term(
        class_tag(*this),
        type,
        vname
    ), vname(vname), global(global) {};

const std::string& FreeVarTerm::getVName() const {
    return vname;
}

} // namespace borealis
