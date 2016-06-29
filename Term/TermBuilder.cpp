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

TermBuilder TermBuilder::operator()(Term::Ptr t) const {
    return { TF, t };
}
TermBuilder TermBuilder::operator()(long long t) const {
    return { TF, TF->getOpaqueConstantTerm((int64_t)t) };
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

TermBuilder TermBuilder::cast(Type::Ptr type) const {
    return { TF, TF->getCastTerm(type, false, term) };
}

TermBuilder operator*(TermFactory::Ptr TF, Term::Ptr term) {
    return {TF, term};
}

TermBuilder operator+(TermBuilder TB, Term::Ptr term) {
    TermBuilder res{TB};
    res.term = res.TF->getBinaryTerm(
        llvm::ArithType::ADD,
        res.term,
        term
    );
    return res;
}

TermBuilder operator+(TermBuilder TB, long long v) {
    TermBuilder res{TB};
    res.term = res.TF->getBinaryTerm(
        llvm::ArithType::ADD,
        res.term,
        TB.TF->getIntTerm(v, TB->getType())
    );
    return res;
}


TermBuilder operator*(TermBuilder TB, Term::Ptr term) {
    TermBuilder res{TB};
    res.term = res.TF->getBinaryTerm(
        llvm::ArithType::MUL,
        res.term,
        term
    );
    return res;
}

TermBuilder operator*(TermBuilder TB, long long v) {
    TermBuilder res{TB};
    res.term = res.TF->getBinaryTerm(
        llvm::ArithType::MUL,
        res.term,
        res.TF->getIntTerm(v, TB.term->getType())
    );
    return res;
}

TermBuilder operator-(TermBuilder TB, Term::Ptr term) {
    TermBuilder res{TB};
    res.term = res.TF->getBinaryTerm(
        llvm::ArithType::SUB,
        res.term,
        term
    );
    return res;
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

Term::Ptr TermBuilder::operator->() const {
    return term;
}
} /* namespace borealis */
