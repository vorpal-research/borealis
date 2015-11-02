/*
 * TermBuilder.h
 *
 *  Created on: May 23, 2013
 *      Author: ice-phoenix
 */

#ifndef TERMBUILDER_H_
#define TERMBUILDER_H_

#include "Term/TermFactory.h"

namespace borealis {

class TermBuilder {

private:

    TermFactory::Ptr TF;
    Term::Ptr term;

public:

    TermBuilder(TermFactory::Ptr TF, Term::Ptr term);
    TermBuilder(const TermBuilder&) = default;
    TermBuilder(TermBuilder&&) = default;

    Term::Ptr operator()() const;

    operator Term::Ptr() const;

    Term::Ptr operator->() const;

    friend TermBuilder operator+(TermBuilder TB, Term::Ptr term);
    friend TermBuilder operator*(TermBuilder TB, Term::Ptr term);
    friend TermBuilder operator-(TermBuilder TB, Term::Ptr term);
    friend TermBuilder operator&&(TermBuilder TB, Term::Ptr term);
    friend TermBuilder operator!=(TermBuilder TB, Term::Ptr term);

    TermBuilder operator*() const;

    TermBuilder uge(Term::Ptr term) const;
    TermBuilder bound() const;

    template<class ...Terms>
    TermBuilder gep(Terms... ptrs) const {
        return { TF, TF->getGepTerm(term, util::make_vector(static_cast<Term::Ptr>(ptrs)...)) };
    }

};

TermBuilder operator*(TermFactory::Ptr TF, Term::Ptr term);

} /* namespace borealis */

#endif /* TERMBUILDER_H_ */
