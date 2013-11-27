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

PredicateStateBuilder::PredicateStateBuilder(
        PredicateStateFactory::Ptr PSF,
        Predicate::Ptr pred) :
        PSF(PSF), State(PSF->Basic() + pred) {};

PredicateState::Ptr PredicateStateBuilder::operator()() const {
    return State->simplify();
}

PredicateStateBuilder& PredicateStateBuilder::operator+=(PredicateState::Ptr s) {
    if (!s->isEmpty()) {
        State = PSF->Chain(State, s);
    }
    return *this;
}

PredicateStateBuilder& PredicateStateBuilder::operator+=(Predicate::Ptr p) {
    State = PSF->Chain(State, p);
    return *this;
}

PredicateStateBuilder operator*(PredicateStateFactory::Ptr PSF, PredicateState::Ptr s) {
    return PredicateStateBuilder{PSF, s};
}

PredicateStateBuilder operator*(PredicateStateFactory::Ptr PSF, Predicate::Ptr p) {
    return PredicateStateBuilder{PSF, p};
}

PredicateStateBuilder operator+(PredicateStateBuilder PSB, PredicateState::Ptr s) {
    PredicateStateBuilder res{PSB};
    res += s;
    return res;
}

PredicateStateBuilder operator+(PredicateStateBuilder PSB, Predicate::Ptr p) {
    PredicateStateBuilder res{PSB};
    res += p;
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
