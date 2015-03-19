/*
 * TermCollector.h
 *
 *  Created on: Mar 18, 2015
 *      Author: ice-phoenix
 */

#ifndef STATE_TRANSFORMER_TERMCOLLECTOR_H_
#define STATE_TRANSFORMER_TERMCOLLECTOR_H_

#include <unordered_set>

#include "State/PredicateState.def"
#include "State/Transformer/Transformer.hpp"

namespace borealis {

class TermCollector : public borealis::Transformer<TermCollector> {

    using Base = borealis::Transformer<TermCollector>;

public:

    TermCollector(FactoryNest FN);

    Term::Ptr transformTerm(Term::Ptr term);

    const std::unordered_set<Term::Ptr>& getTerms() const;

private:

    std::unordered_set<Term::Ptr> terms;

};

} /* namespace borealis */

#endif /* STATE_TRANSFORMER_TERMCOLLECTOR_H_ */
