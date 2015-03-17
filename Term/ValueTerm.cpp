/*
 * ValueTerm.cpp
 *
 *  Created on: Nov 16, 2012
 *      Author: ice-phoenix
 */

#include "Term/ValueTerm.h"

namespace borealis {

ValueTerm::ValueTerm(Type::Ptr type, const std::string& vname, bool global) :
    Term(
        class_tag(*this),
        type,
        vname + (global ? ".global" : "")
    ), vname(vname), global(global) {};

const std::string& ValueTerm::getVName() const {
    return vname;
}

bool ValueTerm::isGlobal() const {
    return global;
}

Term::Ptr ValueTerm::withNewName(const std::string& vname) const {
    TERM_ON_CHANGED(
        this->vname != vname,
        new Self(type, vname, global)
    )
}

} // namespace borealis
