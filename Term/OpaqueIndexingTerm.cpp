/*
 * OpaqueIndexingTerm.cpp
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#include "Term/OpaqueIndexingTerm.h"

namespace borealis {

OpaqueIndexingTerm::OpaqueIndexingTerm(Type::Ptr type, Term::Ptr lhv, Term::Ptr rhv):
    Term(
        class_tag(*this),
        type,
        lhv->getName() + "[" + rhv->getName() + "]"
    ) {
    subterms = { lhv, rhv };
};

Term::Ptr OpaqueIndexingTerm::getLhv() const {
    return subterms[0];
}

Term::Ptr OpaqueIndexingTerm::getRhv() const {
    return subterms[1];
}

} // namespace borealis
