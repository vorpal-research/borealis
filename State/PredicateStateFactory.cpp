/*
 * PredicateStateFactory.cpp
 *
 *  Created on: Apr 30, 2013
 *      Author: ice-phoenix
 */

#include "State/PredicateStateFactory.h"

namespace borealis {

PredicateState::Ptr PredicateStateFactory::Chain(
        PredicateState::Ptr base,
        PredicateState::Ptr curr) {
    return PredicateState::Ptr(
            new PredicateStateChain(base, curr)
    );
}

PredicateState::Ptr PredicateStateFactory::Chain(
        PredicateState::Ptr base,
        Predicate::Ptr pred) {
    return PredicateState::Ptr(
            new PredicateStateChain(base, Basic() + pred)
    );
}

PredicateState::Ptr PredicateStateFactory::Choice(const std::vector<PredicateState::Ptr>& choices) {
    return PredicateState::Ptr(
            new PredicateStateChoice(choices)
    );
}

PredicateState::Ptr PredicateStateFactory::Basic() {
    static PredicateState::Ptr basic(new BasicPredicateState());
    return basic;
}

} /* namespace borealis */
