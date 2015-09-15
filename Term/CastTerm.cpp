/*
 * CastTerm.cpp
 *
 *  Created on: Sep 7, 2015
 *      Author: ice-phoenix
 */

#include "Term/CastTerm.h"

namespace borealis {

CastTerm::CastTerm(Type::Ptr type, bool signExtend, Term::Ptr rhv):
    Term(
        class_tag(*this),
        type,
        "cast(" +
            signExtend ? "+" : "" +
            util::toString(*type) +
            ", " +
            rhv->getName() +
        ")"
    ), signExtend(signExtend) {
    subterms = { rhv };
};

Term::Ptr CastTerm::getRhv() const {
    return subterms[0];
}

bool CastTerm::isSignExtend() const {
    return signExtend;
}

} // namespace borealis
