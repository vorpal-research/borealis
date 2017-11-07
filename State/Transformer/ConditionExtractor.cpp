//
// Created by abdullin on 10/31/17.
//

#include "ConditionExtractor.h"

#include "Util/macros.h"

namespace borealis {

ConditionExtractor::ConditionExtractor(borealis::FactoryNest FN) : Transformer(FN), isVisited(false) {}

PredicateState::Ptr ConditionExtractor::transformBasic(BasicPredicateStatePtr basic) {
    if (not isVisited) {
        conditions = util::viewContainer(basic->getData())
                .filter(LAM(a, a->getType() == PredicateType::PATH))
                .toVector();
        isVisited = true;
    }
    return basic;
}

const std::vector<Predicate::Ptr> ConditionExtractor::getConditions() const {
    return conditions;
}

}   // namespace borealis

#include "Util/unmacros.h"
