/*
 * PredicateState.cpp
 *
 *  Created on: Apr 30, 2013
 *      Author: ice-phoenix
 */

#include "SMT/Z3/Solver.h"
#include "State/PredicateState.h"

namespace borealis {

PredicateState::PredicateState(borealis::id_t predicate_state_type_id) :
        predicate_state_type_id(predicate_state_type_id) {};

PredicateState::~PredicateState() {}

bool PredicateState::isUnreachable() const {
    Z3::ExprFactory z3ef;
    Z3::Solver s(z3ef);

    auto split = this->splitByTypes({PredicateType::PATH});
    return s.isPathImpossible(split.first, split.second);
}

std::ostream& operator<<(std::ostream& s, PredicateState::Ptr state) {
    return s << state->toString();
}

borealis::logging::logstream& operator<<(borealis::logging::logstream& s, PredicateState::Ptr state) {
    return state->dump(s);
}

////////////////////////////////////////////////////////////////////////////////
//
// PredicateState operators
//
////////////////////////////////////////////////////////////////////////////////

PredicateState::Ptr operator+(PredicateState::Ptr state, Predicate::Ptr p) {
    return state->addPredicate(p);
}

PredicateState::Ptr operator<<(PredicateState::Ptr state, const llvm::Value* loc) {
    return state->addVisited(loc);
}

PredicateState::Ptr operator<<(PredicateState::Ptr state, const llvm::Value& loc) {
    return state->addVisited(&loc);
}

} /* namespace borealis */
