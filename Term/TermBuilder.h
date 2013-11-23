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

    friend TermBuilder operator&&(TermBuilder TB, Term::Ptr term);

};

TermBuilder operator*(TermFactory::Ptr TF, Term::Ptr term);

TermBuilder operator&&(TermBuilder TB, Term::Ptr term);

} /* namespace borealis */

#endif /* TERMBUILDER_H_ */
