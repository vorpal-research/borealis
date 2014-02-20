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

class MemoryContextSplitter: public Transformer<MemoryContextSplitter> {

    typedef Transformer<MemoryContextSplitter> Base;

    std::list<Predicate::Ptr> generated;

public:

    MemoryContextSplitter(FactoryNest FN) : Base(FN), generated{} { };

    Term::Ptr transformBoundTerm(BoundTermPtr ptr) {
        auto freeTerm = FN.Term->getValueTerm(ptr->getType(), "$$" + ptr->getName() + "$$");

        auto generatedPredicate = FN.Predicate->getWriteBoundPredicate(ptr->getRhv(), freeTerm);

        generated.push_back(generatedPredicate);

        return freeTerm;
    }

    PredicateState::Ptr getGeneratedPredicates() const noexcept {
        return (FN.State * generated)();
    }
};

} /* namespace borealis */

#endif /* MEMORYCONTEXTSPLITTER_H_ */
