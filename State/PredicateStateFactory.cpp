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
    return PredicateState::Simplified<PredicateStateChain>(
        base, curr
    );
}

PredicateState::Ptr PredicateStateFactory::Chain(
        PredicateState::Ptr base,
        Predicate::Ptr pred) {
    return PredicateState::Simplified<PredicateStateChain>(
        base, Basic() + pred
    );
}

PredicateState::Ptr PredicateStateFactory::Choice(const std::vector<PredicateState::Ptr>& choices) {
    return PredicateState::Simplified<PredicateStateChoice>(
        choices
    );
}

PredicateState::Ptr PredicateStateFactory::Basic() {
    static PredicateState::Ptr basic(new BasicPredicateState());
    return basic;
}

PredicateStateFactory::Ptr PredicateStateFactory::get() {
    static PredicateStateFactory::Ptr instance(new PredicateStateFactory());
    return instance;
}

} /* namespace borealis */
