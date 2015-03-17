/*
 * AxiomTerm.cpp
 *
 *  Created on: Aug 28, 2013
 *      Author: sam
 */

#include "Term/AxiomTerm.h"

#include "Util/macros.h"

namespace borealis {

AxiomTerm::AxiomTerm(Type::Ptr type, Term::Ptr lhv, Term::Ptr rhv):
    Term(
        class_tag(*this),
        type,
        "(" + lhv->getName() + " with axiom " + rhv->getName() + ")"
    ) {
    ASSERT(llvm::isa<type::Bool>(rhv->getType()), "Attempt to add a non-Bool axiom term");
    subterms = { lhv, rhv };
};

Term::Ptr AxiomTerm::getLhv() const {
    return subterms[0];
}

Term::Ptr AxiomTerm::getRhv() const {
    return subterms[1];
}

} // namespace borealis

#include "Util/unmacros.h"
