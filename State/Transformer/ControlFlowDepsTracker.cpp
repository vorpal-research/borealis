//
// Created by belyaev on 10/31/16.
//

#include "State/Transformer/ControlFlowDepsTracker.h"

namespace borealis {



ControlFlowDepsTracker::ControlFlowDepsTracker(borealis::FactoryNest FN) : Transformer(FN) {}

Predicate::Ptr ControlFlowDepsTracker::transformBase(Predicate::Ptr pred) {
    if(pred->getType() != PredicateType::STATE) {
        currentDominators_.insert(pred);
    }
    else dominatorMap_[pred].insert(std::begin(currentDominators_), std::end(currentDominators_));
    return pred;
}

PredicateState::Ptr ControlFlowDepsTracker::transformChoice(Transformer::PredicateStateChoicePtr choice) {

    auto currentDominators = currentDominators_;
    auto totalDominators = currentDominators_;
    for(auto&& branch: choice->getChoices()) {
        currentDominators_ = currentDominators;
        transform(branch);
        totalDominators.insert(std::begin(currentDominators_), std::end(currentDominators_));
    }

    currentDominators_ = std::move(totalDominators);

    return choice;
}

std::unordered_set<Predicate::Ptr, PredicateHash, PredicateEquals>& ControlFlowDepsTracker::getDominatingPaths(Predicate::Ptr state){
    static std::unordered_set<Predicate::Ptr, PredicateHash, PredicateEquals> empty;
    return util::at(dominatorMap_, state).getOrElse(empty);
}

std::unordered_set<Predicate::Ptr, PredicateHash, PredicateEquals>& ControlFlowDepsTracker::getFinalPaths() {
    return currentDominators_;
}


} /* namespace borealis */

