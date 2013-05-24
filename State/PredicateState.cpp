/*
 * PredicateState.cpp
 *
 *  Created on: Apr 30, 2013
 *      Author: ice-phoenix
 */

#include "Solver/Z3Solver.h"
#include "State/PredicateState.h"

namespace borealis {

PredicateState::PredicateState(borealis::id_t predicate_state_type_id) :
        predicate_state_type_id(predicate_state_type_id) {};

PredicateState::~PredicateState() {}

bool PredicateState::isUnreachable() const {
    z3::context ctx;
    Z3ExprFactory z3ef(ctx);
    Z3Solver s(z3ef);

    auto split = this->splitByTypes({PredicateType::PATH});
    return s.isPathImpossible(split.first, split.second);
}

std::ostream& operator<<(std::ostream& s, PredicateState::Ptr state) {
    return s << state->toString();
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
