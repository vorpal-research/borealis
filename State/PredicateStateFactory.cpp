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
    )->simplify();
}

PredicateState::Ptr PredicateStateFactory::Chain(
        PredicateState::Ptr base,
        Predicate::Ptr pred) {
    return PredicateState::Ptr(
            new PredicateStateChain(base, Basic() + pred)
    )->simplify();
}

PredicateState::Ptr PredicateStateFactory::Choice(const std::vector<PredicateState::Ptr>& choices) {
    return PredicateState::Ptr(
            new PredicateStateChoice(choices)
    )->simplify();
}

PredicateState::Ptr PredicateStateFactory::Basic() {
    static PredicateState::Ptr basic(new BasicPredicateState());
    return basic;
}

} /* namespace borealis */
