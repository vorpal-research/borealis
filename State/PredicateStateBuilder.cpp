/*
 * PredicateStateBuilder.cpp
 *
 *  Created on: May 23, 2013
 *      Author: ice-phoenix
 */

#include "State/PredicateStateBuilder.h"

namespace borealis {

PredicateStateBuilder::PredicateStateBuilder(PredicateStateFactory::Ptr PSF) :
        PSF(PSF), State(nullptr) {};

PredicateState::Ptr PredicateStateBuilder::operator()() const {
    return State;
}

PredicateStateBuilder operator&&(PredicateStateBuilder PSB, PredicateState::Ptr s) {
    PredicateStateBuilder res{PSB};
    if (!s->isEmpty()) {
        res.State = res.State ? res.PSF->Chain(res.State, s) : s;
    }
    return res;
}

PredicateStateBuilder operator+(PredicateStateBuilder PSB, PredicateState::Ptr s) {
    PredicateStateBuilder res{PSB};
    if (!s->isEmpty()) {
        res.State = res.State ? res.PSF->Chain(res.State, s) : s;
    }
    return res;
}

PredicateStateBuilder operator&&(PredicateStateBuilder PSB, Predicate::Ptr p) {
    PredicateStateBuilder res{PSB};
    res.State = res.State ? res.State + p : res.PSF->Basic() + p;
    return res;
}

PredicateStateBuilder operator+(PredicateStateBuilder PSB, Predicate::Ptr p) {
    PredicateStateBuilder res{PSB};
    res.State = res.State ? res.State + p : res.PSF->Basic() + p;
    return res;
}

PredicateStateBuilder operator<<(PredicateStateBuilder PSB, const llvm::Value* loc) {
    PredicateStateBuilder res{PSB};
    res.State = res.State ? res.State << loc : res.PSF->Basic() << loc;
    return res;
}

PredicateStateBuilder operator<<(PredicateStateBuilder PSB, const llvm::Value& loc) {
    PredicateStateBuilder res{PSB};
    res.State = res.State ? res.State << loc : res.PSF->Basic() << loc;
    return res;
}

} /* namespace borealis */
