/*
 * ReadPropertyTerm.cpp
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#include "Term/ReadPropertyTerm.h"

namespace borealis {

ReadPropertyTerm::ReadPropertyTerm(Type::Ptr type, Term::Ptr propName, Term::Ptr rhv):
    Term(
        class_tag(*this),
        type,
        "read(" + propName->getName() + "," + rhv->getName() + ")"
    ) {
    subterms = { rhv, propName };
};

Term::Ptr ReadPropertyTerm::getRhv() const {
    return subterms[0];
}

Term::Ptr ReadPropertyTerm::getPropertyName() const {
    return subterms[1];
}

} // namespace borealis
