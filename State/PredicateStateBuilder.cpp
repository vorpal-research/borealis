/*
 * PredicateStateBuilder.cpp
 *
 *  Created on: May 23, 2013
 *      Author: ice-phoenix
 */

#include "State/PredicateStateBuilder.h"
#include "State/Transformer/Retyper.h"

namespace borealis {

PredicateStateBuilder::PredicateStateBuilder(
        PredicateStateFactory::Ptr PSF,
        PredicateState::Ptr state) :
        PSF(PSF), State(state) {};

PredicateStateBuilder::PredicateStateBuilder(
    PredicateStateFactory::Ptr PSF) :
    PSF(PSF), State(PSF->Basic()) {};

PredicateStateBuilder::PredicateStateBuilder(
        PredicateStateFactory::Ptr PSF,
        Predicate::Ptr pred) :
        PSF(PSF), State(PSF->Basic() + pred) {};

PredicateState::Ptr PredicateStateBuilder::operator()() const {
    return Retyper(FactoryNest()).transform(State);
}

PredicateState::Ptr PredicateStateBuilder::apply() const {
    return State;
}

PredicateStateBuilder& PredicateStateBuilder::operator+=(PredicateState::Ptr s) {
    if(s->isEmpty()) return *this;
    if(State->isEmpty()) {
        State = s;
        return *this;
    }

    State = PSF->Chain(State, s);

    return *this;
}

PredicateStateBuilder& PredicateStateBuilder::operator+=(Predicate::Ptr p) {
    return operator+=(PSF->Basic() + p);
}

PredicateStateBuilder& PredicateStateBuilder::operator<<=(const Locus& locus) {
    State = State << locus;
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

PredicateStateBuilder operator<<(PredicateStateBuilder PSB, const Locus& locus) {
    PredicateStateBuilder res{PSB};
    res <<= locus;
    return res;
}

} /* namespace borealis */
