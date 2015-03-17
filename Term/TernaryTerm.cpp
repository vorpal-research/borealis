/*
 * TernaryTerm.cpp
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#include "Term/TernaryTerm.h"

namespace borealis {

TernaryTerm::TernaryTerm(Type::Ptr type, Term::Ptr cnd, Term::Ptr tru, Term::Ptr fls):
    Term(
        class_tag(*this),
        type,
        "(" + cnd->getName() + " ? " + tru->getName() + " : " + fls->getName() + ")"
    ) {
    subterms = { cnd, tru, fls };
};

Term::Ptr TernaryTerm::getCnd() const {
    return subterms[0];
}

Term::Ptr TernaryTerm::getTru() const {
    return subterms[1];
}

Term::Ptr TernaryTerm::getFls() const {
    return subterms[2];
}

} // namespace borealis
