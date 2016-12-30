//
// Created by belyaev on 10/31/16.
//

#ifndef CONTROLFLOWDEPSTRACKER_H
#define CONTROLFLOWDEPSTRACKER_H

#include "State/Transformer/Transformer.hpp"

#include "Util/hamt.hpp"

namespace borealis {


class ControlFlowDepsTracker: public Transformer<ControlFlowDepsTracker> {

    using Base = borealis::Transformer<ControlFlowDepsTracker>;

public:
    using PredicateSet = util::hamt_set<Predicate::Ptr, PredicateHash, PredicateEquals>;

private:
    std::unordered_map<Predicate::Ptr, PredicateSet, PredicateShallowHash, PredicateShallowEquals> dominatorMap_;
    PredicateSet currentDominators_;

public:
    ControlFlowDepsTracker(FactoryNest FN);

    // we don't need to go below predicates
    using Base::transformBase;
    Predicate::Ptr transformBase(Predicate::Ptr pred);

    PredicateState::Ptr transformChoice(PredicateStateChoicePtr choice);

    PredicateState::Ptr transform(PredicateState::Ptr stt);

    void cleanup();

    PredicateSet& getDominatingPaths(Predicate::Ptr state);
    PredicateSet& getFinalPaths();

    void reset() {
        dominatorMap_.clear();
        currentDominators_ = PredicateSet();
    }
};

} /* namespace borealis */

#endif //CONTROLFLOWDEPSTRACKER_H
