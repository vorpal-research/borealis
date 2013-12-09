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
        TF(TF), term(term) {};

Term::Ptr TermBuilder::operator()() const {
    return term;
}

TermBuilder::operator Term::Ptr() const {
    return term;
}

TermBuilder TermBuilder::operator*() const {
    return { TF, TF->getLoadTerm(term) };
}

TermBuilder TermBuilder::uge(Term::Ptr that) const {
    return { TF, TF->getCmpTerm(llvm::ConditionType::UGE, term, that) };
}

TermBuilder TermBuilder::bound() const {
    return { TF, TF->getBoundTerm(term) };
}

TermBuilder operator*(TermFactory::Ptr TF, Term::Ptr term) {
    return {TF, term};
}

TermBuilder operator&&(TermBuilder TB, Term::Ptr term) {
    TermBuilder res{TB};
    res.term = res.TF->getBinaryTerm(
        llvm::ArithType::LAND,
        res.term,
        term
    );
    return res;
}

TermBuilder operator!=(TermBuilder TB, Term::Ptr term) {
    TermBuilder res{TB};
    res.term = res.TF->getCmpTerm(
        llvm::ConditionType::NEQ,
        res.term,
        term
    );
    return res;
}

} /* namespace borealis */
