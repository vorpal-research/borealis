/*
 * StateSlicer.h
 *
 *  Created on: Mar 16, 2015
 *      Author: ice-phoenix
 */

#ifndef STATE_TRANSFORMER_STATESLICER_H_
#define STATE_TRANSFORMER_STATESLICER_H_

#include <unordered_set>

#include "State/PredicateState.def"
#include "State/Transformer/Transformer.hpp"

namespace borealis {

class StateSlicer : public borealis::Transformer<StateSlicer> {

    using Base = borealis::Transformer<StateSlicer>;

public:

    StateSlicer(FactoryNest FN, PredicateState::Ptr query);

    Predicate::Ptr transformPredicate(Predicate::Ptr pred);

private:

    PredicateState::Ptr query;

    std::unordered_set<Term::Ptr> sliceVars;
    std::unordered_set<Term::Ptr> slicePtrs;

    void init();

};

} /* namespace borealis */

#endif /* STATE_TRANSFORMER_STATESLICER_H_ */
