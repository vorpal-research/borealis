/*
 * OpaqueBuiltinTerm.cpp
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#include "Term/OpaqueBuiltinTerm.h"

namespace borealis {

OpaqueBuiltinTerm::OpaqueBuiltinTerm(Type::Ptr type, const std::string& vname):
    Term(
        class_tag(*this),
        type,
        "\\" + vname
    ), vname(vname) {};

const std::string& OpaqueBuiltinTerm::getVName() const {
    return vname;
}

} // namespace borealis
