/*
 * PredicateState.cpp
 *
 *  Created on: Apr 30, 2013
 *      Author: ice-phoenix
 */

#include "SMT/MathSAT/Solver.h"
#include "SMT/Z3/Solver.h"
#include "State/PredicateState.h"

namespace borealis {

PredicateState::PredicateState(id_t classTag) :
        ClassTag(classTag) {};

bool PredicateState::isUnreachableIn(unsigned long long memoryStart) const {

#if defined USE_MATHSAT_SOLVER
        MathSAT::ExprFactory ef;
        MathSAT::Solver s(ef, memoryStart);
#else
        Z3::ExprFactory ef;
        Z3::Solver s(ef, memoryStart);
#endif

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
