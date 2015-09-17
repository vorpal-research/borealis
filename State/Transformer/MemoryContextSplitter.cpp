/*
 * MemoryContextSplitter.cpp
 *
 *  Created on: Feb 20, 2014
 *      Author: belyaev
 */

#include "State/Transformer/MemoryContextSplitter.h"

namespace borealis {

MemoryContextSplitter::MemoryContextSplitter(FactoryNest FN) : Base(FN) {}

Term::Ptr MemoryContextSplitter::transformBoundTerm(BoundTermPtr ptr) {
    auto&& freeTerm = FN.Term->getValueTerm(ptr->getType(), "$$" + ptr->getName() + "$$");

    if (not util::contains(processedTerms, freeTerm)) {
        processedTerms.insert(freeTerm);
        auto&& generatedPredicate = FN.Predicate->getWriteBoundPredicate(ptr->getRhv(), freeTerm);
        generated.push_back(generatedPredicate);
    }

    return freeTerm;
}

PredicateState::Ptr MemoryContextSplitter::getGeneratedPredicates() const {
    return (FN.State * generated)();
}

} /* namespace borealis */
