/*
 * PredicateStateBuilder.cpp
 *
 *  Created on: May 23, 2013
 *      Author: ice-phoenix
 */

#include "State/PredicateStateBuilder.h"

namespace borealis {

PredicateStateBuilder::PredicateStateBuilder(
        PredicateStateFactory::Ptr PSF,
        PredicateState::Ptr state) :
        PSF(PSF), State(state) {};

PredicateState::Ptr PredicateStateBuilder::operator()() const {
    return State;
}

PredicateStateBuilder operator*(PredicateStateFactory::Ptr PSF, PredicateState::Ptr s) {
    return PredicateStateBuilder{PSF, s};
}

PredicateStateBuilder operator*(PredicateStateFactory::Ptr PSF, Predicate::Ptr p) {
    return PredicateStateBuilder{PSF, PSF->Basic()} + p;
}

PredicateStateBuilder operator+(PredicateStateBuilder PSB, PredicateState::Ptr s) {
    PredicateStateBuilder res{PSB};
    if (!s->isEmpty()) {
        res.State = res.PSF->Chain(res.State, s);
    }
    return res;
}

PredicateStateBuilder operator+(PredicateStateBuilder PSB, Predicate::Ptr p) {
    PredicateStateBuilder res{PSB};
    res.State = res.State + p;
    return res;
}

PredicateStateBuilder operator<<(PredicateStateBuilder PSB, const llvm::Value* loc) {
    PredicateStateBuilder res{PSB};
    res.State = res.State << loc;
    return res;
}

PredicateStateBuilder operator<<(PredicateStateBuilder PSB, const llvm::Value& loc) {
    PredicateStateBuilder res{PSB};
    res.State = res.State << loc;
    return res;
}

} /* namespace borealis */
