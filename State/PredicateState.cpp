/*
 * PredicateState.cpp
 *
 *  Created on: Apr 30, 2013
 *      Author: ice-phoenix
 */

#include "SMT/MathSAT/Solver.h"
#include "SMT/Z3/Solver.h"
#include "State/PredicateState.h"

#include "Util/macros.h"

namespace borealis {

PredicateState::PredicateState(id_t classTag) : ClassTag(classTag) {};

PredicateState::Ptr PredicateState::self() const {
    return this->shared_from_this();
}

PredicateState::Ptr PredicateState::fmap(FMapper) const {
    BYE_BYE(PredicateState::Ptr, "Should not be called!");
}

PredicateState::Ptr PredicateState::map(Mapper m) const {
    return fmap([&](PredicateState::Ptr s) { return s->map(m); });
}

PredicateState::Ptr PredicateState::filterByTypes(std::initializer_list<PredicateType> types) const {
    return fmap([&](PredicateState::Ptr s) { return s->filterByTypes(types); });
}

PredicateState::Ptr PredicateState::filter(Filterer f) const {
    return fmap([&](PredicateState::Ptr s) { return s->filter(f); });
}

PredicateState::Ptr PredicateState::reverse() const {
    return fmap([&](PredicateState::Ptr s) { return s->reverse(); });
}

bool PredicateState::isUnreachableIn(unsigned long long memoryStart, unsigned long long memoryEnd) const {

#if defined USE_MATHSAT_SOLVER
        MathSAT::ExprFactory ef;
        MathSAT::Solver s(ef, memoryStart, memoryEnd);
#else
        Z3::ExprFactory ef;
        Z3::Solver s(ef, memoryStart, memoryEnd);
#endif

    auto&& split = splitByTypes({PredicateType::PATH});
    return s.isPathImpossible(split.first, split.second).isUnsat();
}

bool PredicateState::equals(const PredicateState* other) const {
    if (other == nullptr) return false;
    return classTag == other->classTag;
}

bool operator==(const PredicateState& a, const PredicateState& b) {
    if (&a == &b) return true;
    else return a.equals(&b);
}

std::ostream& operator<<(std::ostream& s, PredicateState::Ptr state) {
    return s << state->toString();
}

borealis::logging::logstream& operator<<(borealis::logging::logstream& s, PredicateState::Ptr state) {
    return state->dump(s);
}

PredicateState::Ptr operator+(PredicateState::Ptr state, Predicate::Ptr pred) {
    return state->addPredicate(pred);
}

PredicateState::Ptr operator<<(PredicateState::Ptr state, const Locus& locus) {
    return state->addVisited(locus);
}

} /* namespace borealis */

#include "Util/unmacros.h"
