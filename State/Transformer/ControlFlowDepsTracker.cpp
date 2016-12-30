//
// Created by belyaev on 10/31/16.
//

#include "State/Transformer/ControlFlowDepsTracker.h"

namespace borealis {



ControlFlowDepsTracker::ControlFlowDepsTracker(borealis::FactoryNest FN) : Transformer(FN) {}

Predicate::Ptr ControlFlowDepsTracker::transformBase(Predicate::Ptr pred) {
    if(pred->getType() != PredicateType::STATE) {
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

util::hamt_set<Predicate::Ptr, PredicateHash, PredicateEquals>& ControlFlowDepsTracker::getDominatingPaths(Predicate::Ptr state){
    static util::hamt_set<Predicate::Ptr, PredicateHash, PredicateEquals> empty;
    return util::at(dominatorMap_, state).getOrElse(empty);
}

util::hamt_set<Predicate::Ptr, PredicateHash, PredicateEquals>& ControlFlowDepsTracker::getFinalPaths() {
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

void ControlFlowDepsTracker::cleanup() {
//    auto save = currentDominators_;
////    std::cerr << "Current doms" << std::endl;
////    save.dump();
//    save.foreach(
//        [this](auto&& p) {
//            auto notP = inverse(FN, p);
//            if(!notP) return; // not our client
////            std::cerr << "Erasing " << p << std::endl;
//            for(auto&& entry: dominatorMap_) {
//                auto&& set = entry.second;
//                if(set.count(p) && set.count(notP)) {
//                    set = set.erase(p).erase(notP);
////                    std::cerr << "Erased: " << std::endl
////                              << "   " << p << std::endl
////                              << "   " << notP << std::endl;
//                }
//            }
//
//            if(currentDominators_.count(p) && currentDominators_.count(notP)) {
//                currentDominators_ = currentDominators_.erase(p).erase(notP);
//            }
//        }
//    );
}

PredicateState::Ptr ControlFlowDepsTracker::transform(PredicateState::Ptr stt) {
    auto result = Base::transform(stt);
    cleanup();
    return result;
}


} /* namespace borealis */

