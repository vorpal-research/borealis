/*
 * TermBuilder.cpp
 *
 *  Created on: May 23, 2013
 *      Author: ice-phoenix
 */

#include "Term/TermBuilder.h"

namespace borealis {

TermBuilder::TermBuilder(
        TermFactory::Ptr TF,
        Term::Ptr term) :
        TF(TF), Term(term) {};

Term::Ptr TermBuilder::operator()() const {
    return Term;
}

TermBuilder operator*(TermFactory::Ptr TF, Term::Ptr term) {
    return TermBuilder{TF, term};
}

TermBuilder operator&&(TermBuilder TB, Term::Ptr term) {
    TermBuilder res{TB};
    res.Term = res.TF->getBinaryTerm(
        llvm::ArithType::LAND,
        res.Term,
        term
    );
    return res;
}

} /* namespace borealis */
