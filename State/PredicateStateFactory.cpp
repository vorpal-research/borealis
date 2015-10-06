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
    if (base->isEmpty()) {
        return curr;
    } else if (curr->isEmpty()) {
        return base;
    } else {
        return PredicateState::Simplified<PredicateStateChain>(
            base, curr
        );
    }
}

PredicateState::Ptr PredicateStateFactory::Chain(
        PredicateState::Ptr base,
        Predicate::Ptr pred) {
    return PredicateState::Simplified<PredicateStateChain>(
        base, Basic() + pred
    );
}

PredicateState::Ptr PredicateStateFactory::Choice(const std::vector<PredicateState::Ptr>& choices) {
    return 1 == choices.size()
           ? choices.front()
           : PredicateState::Simplified<PredicateStateChoice>(choices);
}

PredicateState::Ptr PredicateStateFactory::Choice(std::vector<PredicateState::Ptr>&& choices) {
    auto&& moved = std::move(choices);
    return 1 == moved.size()
           ? moved.front()
           : PredicateState::Simplified<PredicateStateChoice>(moved);
}

PredicateState::Ptr PredicateStateFactory::Basic() {
    static PredicateState::Ptr basic(new BasicPredicateState());
    return basic;
}

PredicateState::Ptr PredicateStateFactory::Basic(const std::vector<Predicate::Ptr>& data) {
    return PredicateState::Simplified<BasicPredicateState>(
        data
    );
}

PredicateState::Ptr PredicateStateFactory::Basic(std::vector<Predicate::Ptr>&& data) {
    return PredicateState::Simplified<BasicPredicateState>(
        std::move(data)
    );
}

PredicateStateFactory::Ptr PredicateStateFactory::get() {
    static PredicateStateFactory::Ptr instance(new PredicateStateFactory());
    return instance;
}

} /* namespace borealis */
