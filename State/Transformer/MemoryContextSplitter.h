/*
 * MemoryContextSplitter.h
 *
 *  Created on: Feb 20, 2014
 *      Author: belyaev
 */

#ifndef MEMORYCONTEXTSPLITTER_H_
#define MEMORYCONTEXTSPLITTER_H_

#include "State/PredicateStateBuilder.h"
#include "State/Transformer/Transformer.hpp"

namespace borealis {

class MemoryContextSplitter : public Transformer<MemoryContextSplitter> {

    using Base = Transformer<MemoryContextSplitter>;

    std::list<Predicate::Ptr> generated;
    Term::Set processedTerms;

public:

    MemoryContextSplitter(FactoryNest FN);

    Term::Ptr transformBoundTerm(BoundTermPtr ptr);

    PredicateState::Ptr getGeneratedPredicates() const;

};

} /* namespace borealis */

#endif /* MEMORYCONTEXTSPLITTER_H_ */
