/*
 * SignTerm.cpp
 *
 *  Created on: Aug 28, 2013
 *      Author: sam
 */

#include "Term/SignTerm.h"

namespace borealis {

SignTerm::SignTerm(Type::Ptr type, Term::Ptr rhv):
    Term(
        class_tag(*this),
        type,
        "sign(" + rhv->getName() + ")"
    ) {
    subterms = { rhv };
};

Term::Ptr SignTerm::getRhv() const {
    return subterms[0];
}

} // namespace borealis
