/*
 * Simplifier.h
 *
 *  Created on: Nov 20, 2012
 *      Author: ice-phoenix
 */

#ifndef SIMPLIFIER_H_
#define SIMPLIFIER_H_

#include "State/Transformer/Transformer.hpp"

namespace borealis {

class Simplifier : public borealis::Transformer<Simplifier> {

    typedef borealis::Transformer<Simplifier> Base;

    unsigned int level;
    std::vector<Term::Ptr> axs;

public:

    Simplifier(FactoryNest FN) : Base(FN), level(0U) {}

    using Base::transformBase;

    Term::Ptr transformBase(Term::Ptr term);

    Term::Ptr transformAxiomTerm(AxiomTermPtr t);

    Term::Ptr transformUnaryTerm(UnaryTermPtr t);

};

} /* namespace borealis */

#endif /* SIMPLIFIER_H_ */
