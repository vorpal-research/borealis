/*
 * StateOptimizer.h
 *
 *  Created on: Nov 20, 2012
 *      Author: ice-phoenix
 */

#ifndef STATEOPTIMIZER_H_
#define STATEOPTIMIZER_H_

#include "State/PredicateState.def"
#include "State/Transformer/CachingTransformer.hpp"

#include "Util/macros.h"

namespace borealis {

class StateOptimizer : public borealis::CachingTransformer<StateOptimizer> {

    using Base = borealis::CachingTransformer<StateOptimizer>;

public:

    StateOptimizer(FactoryNest FN);

    // TODO akhin: Optimize transformer framework

    PredicateState::Ptr transformPredicateStateChain(PredicateStateChainPtr ps);

    PredicateState::Ptr transformBasic(BasicPredicateStatePtr ps);

    using Base::transformBase;
    Predicate::Ptr transformBase(Predicate::Ptr pred);

private:

    std::unordered_map<std::pair<PredicateState::Ptr, PredicateState::Ptr>, PredicateState::Ptr> mergeCache;

    PredicateState::Ptr merge(PredicateState::Ptr a, PredicateState::Ptr b);

};

} /* namespace borealis */

#include "Util/unmacros.h"

#endif /* STATEOPTIMIZER_H_ */
