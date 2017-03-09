//
// Created by belyaev on 10/31/16.
//

#include "State/Transformer/ControlFlowDepsTracker.h"

namespace borealis {

ControlFlowDepsTracker::ControlFlowDepsTracker(borealis::FactoryNest FN) : Transformer(FN) {}

Predicate::Ptr ControlFlowDepsTracker::transformBase(Predicate::Ptr pred) {
    if(pred->getType() != PredicateType::STATE && pred->getType() != PredicateType::INVARIANT ) {
        currentDominators_ = currentDominators_.insert(pred);
    }

    dominatorMap_[pred] = unite(dominatorMap_[pred], currentDominators_);
    return pred;
}

PredicateState::Ptr ControlFlowDepsTracker::transformChoice(Transformer::PredicateStateChoicePtr choice) {

    auto currentDominators = currentDominators_;
    auto totalDominators = currentDominators_;
    for(auto&& branch: choice->getChoices()) {
        currentDominators_ = currentDominators;
        Base::transform(branch);
        totalDominators = unite(totalDominators, currentDominators_);
    }

    currentDominators_ = std::move(totalDominators);
    return choice;
}

ControlFlowDepsTracker::PredicateSet& ControlFlowDepsTracker::getDominatingPaths(Predicate::Ptr state){
    static ControlFlowDepsTracker::PredicateSet empty;
    return util::at(dominatorMap_, state).getOrElse(empty);
}

ControlFlowDepsTracker::PredicateSet& ControlFlowDepsTracker::getFinalPaths() {
    return currentDominators_;
}

static Predicate::Ptr inverse(FactoryNest& FN, const Predicate::Ptr& predicate) {
    using namespace functional_hell::matchers;
    using namespace functional_hell::matchers::placeholders;
    if(auto m = $EqualityPredicate(_1, $OpaqueBoolConstantTerm(true)) >> predicate) {
        return FN.Predicate->getEqualityPredicate(m->_1, FN.Term->getFalseTerm(), predicate->getLocation(), predicate->getType());
    }
    if(auto m = $EqualityPredicate(_1, $OpaqueBoolConstantTerm(false)) >> predicate) {
        return FN.Predicate->getEqualityPredicate(m->_1, FN.Term->getTrueTerm(), predicate->getLocation(), predicate->getType());
    }
    return nullptr;
}

void ControlFlowDepsTracker::cleanup() {}

PredicateState::Ptr ControlFlowDepsTracker::transform(PredicateState::Ptr stt) {
    auto result = Base::transform(stt);
    cleanup();
    return result;
}


} /* namespace borealis */

