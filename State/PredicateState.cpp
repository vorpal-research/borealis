/*
 * PredicateState.cpp
 *
 *  Created on: Apr 30, 2013
 *      Author: ice-phoenix
 */

#include "State/PredicateState.h"

namespace borealis {

PredicateState::PredicateState(borealis::id_t predicate_state_type_id) :
        predicate_state_type_id(predicate_state_type_id) {};

PredicateState::~PredicateState() {}

std::ostream& operator<<(std::ostream& s, PredicateState::Ptr state) {
    return s << state->toString();
}

////////////////////////////////////////////////////////////////////////////////
//
// PredicateState operators
//
////////////////////////////////////////////////////////////////////////////////

PredicateState::Ptr operator&&(PredicateState::Ptr state, Predicate::Ptr p) {
    return state->addPredicate(p);
}

PredicateState::Ptr operator+(PredicateState::Ptr state, Predicate::Ptr p) {
    return state->addPredicate(p);
}

PredicateState::Ptr operator&&(PredicateState::Ptr a, PredicateState::Ptr b) {
    return a->addAll(b);
}

PredicateState::Ptr operator+(PredicateState::Ptr a, PredicateState::Ptr b) {
    return a->addAll(b);
}

PredicateState::Ptr operator<<(PredicateState::Ptr state, const llvm::Value* loc) {
    return state->addVisited(loc);
}

PredicateState::Ptr operator<<(PredicateState::Ptr state, const llvm::Value& loc) {
    return state->addVisited(&loc);
}

} /* namespace borealis */
