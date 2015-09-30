/*
 * OpaqueNamedConstantTerm.cpp
 *
 *  Created on: Sep 29, 2015
 *      Author: belyaev
 */

#include "Term/OpaqueNamedConstantTerm.h"

namespace borealis {

OpaqueNamedConstantTerm::OpaqueNamedConstantTerm(Type::Ptr type, const std::string& vname):
    Term(
        class_tag(*this),
        type,
        "@" + vname
    ), vname(vname) {};

const std::string& OpaqueNamedConstantTerm::getVName() const {
    return vname;
}

} // namespace borealis
