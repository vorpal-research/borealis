/*
 * OpaqueVarTerm.cpp
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#include "Term/OpaqueVarTerm.h"

namespace borealis {

OpaqueVarTerm::OpaqueVarTerm(Type::Ptr type, const std::string& vname):
    Term(
        class_tag(*this),
        type,
        vname
    ), vname(vname) {};

const std::string& OpaqueVarTerm::getVName() const {
    return vname;
}

} // namespace borealis
