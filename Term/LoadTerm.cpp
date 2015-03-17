/*
 * LoadTerm.cpp
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#include "Term/LoadTerm.h"

namespace borealis {

LoadTerm::LoadTerm(Type::Ptr type, Term::Ptr rhv, bool retypable):
    Term(
        class_tag(*this),
        type,
        "*(" + rhv->getName() + ")",
        retypable
    ) {
    subterms = { rhv };
};

Term::Ptr LoadTerm::getRhv() const {
    return subterms[0];
}

} // namespace borealis
