/*
 * StateOptimizer.h
 *
 *  Created on: Nov 20, 2012
 *      Author: ice-phoenix
 */

#ifndef STATEOPTIMIZER_H_
#define STATEOPTIMIZER_H_

#include "State/PredicateState.def"
#include "State/Transformer/Transformer.hpp"

#include "Util/macros.h"

namespace borealis {

class StateOptimizer : public borealis::Transformer<StateOptimizer> {

    using Base = borealis::Transformer<StateOptimizer>;

public:

    StateOptimizer(FactoryNest FN);

    PredicateState::Ptr transformPredicateStateChain(PredicateStateChainPtr ps);

    using Base::transformBase;
    Predicate::Ptr transformBase(Predicate::Ptr pred);

private:

    PredicateState::Ptr merge(PredicateState::Ptr a, PredicateState::Ptr b);

};

} /* namespace borealis */

#include "Util/unmacros.h"

#endif /* STATEOPTIMIZER_H_ */
