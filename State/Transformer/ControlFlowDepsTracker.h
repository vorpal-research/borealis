//
// Created by belyaev on 10/31/16.
//

#ifndef CONTROLFLOWDEPSTRACKER_H
#define CONTROLFLOWDEPSTRACKER_H

#include "State/Transformer/Transformer.hpp"

#include "Util/split_join.hpp"

namespace borealis {


class ControlFlowDepsTracker: public Transformer<ControlFlowDepsTracker> {

    using Base = borealis::Transformer<ControlFlowDepsTracker>;

    std::unordered_map<Predicate::Ptr, std::unordered_set<Predicate::Ptr, PredicateHash, PredicateEquals>, PredicateShallowHash, PredicateShallowEquals> dominatorMap_;
    std::unordered_set<Predicate::Ptr, PredicateHash, PredicateEquals> currentDominators_;

public:
    ControlFlowDepsTracker(FactoryNest FN);

    // we don't need to go below predicates
    using Base::transformBase;
    Predicate::Ptr transformBase(Predicate::Ptr pred);

    PredicateState::Ptr transformChoice(PredicateStateChoicePtr choice);

    std::unordered_set<Predicate::Ptr, PredicateHash, PredicateEquals>& getDominatingPaths(Predicate::Ptr state);
    std::unordered_set<Predicate::Ptr, PredicateHash, PredicateEquals>& getFinalPaths();

    void reset() {
        dominatorMap_.clear();
        currentDominators_.clear();
    }
};

} /* namespace borealis */

#endif //CONTROLFLOWDEPSTRACKER_H
