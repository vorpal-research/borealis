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
        ""
    ), signExtend(signExtend) {
    subterms = { rhv };
    update();
};

void CastTerm::update() {
    name =
        "cast(" +
            std::string(signExtend ? "+" : "") +
            util::toString(*type) +
            ", " +
            getRhv()->getName() +
        ")";
}

Term::Ptr CastTerm::getRhv() const {
    return subterms[0];
}

bool CastTerm::isSignExtend() const {
    return signExtend;
}

} // namespace borealis
