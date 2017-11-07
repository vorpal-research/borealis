//
// Created by abdullin on 10/31/17.
//

#ifndef BOREALIS_CONDITIONEXTRACTOR_H
#define BOREALIS_CONDITIONEXTRACTOR_H

#include "Transformer.hpp"

namespace borealis {

class ConditionExtractor : public Transformer<ConditionExtractor> {
public:
    using Base = Transformer<ConditionExtractor>;

    ConditionExtractor(FactoryNest FN);

    PredicateState::Ptr transformBasic(BasicPredicateStatePtr basic);

    const std::vector<Predicate::Ptr> getConditions() const;

private:

    bool isVisited;
    std::vector<Predicate::Ptr> conditions;
};

}   // namespace borealis


#endif //BOREALIS_CONDITIONEXTRACTOR_H
