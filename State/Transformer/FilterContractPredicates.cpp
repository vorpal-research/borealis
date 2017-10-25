//
// Created by abdullin on 10/24/17.
//

#include "FilterContractPredicates.h"

#include "Util/macros.h"

namespace borealis {

FilterContractPredicates::FilterContractPredicates(borealis::FactoryNest FN) : Transformer(FN) {}

PredicateState::Ptr FilterContractPredicates::transform(PredicateState::Ptr ps) {
    return Base::transform(ps)
            ->filter(LAM(p, !!p))
            ->simplify();
}

Predicate::Ptr FilterContractPredicates::transformPredicate(Predicate::Ptr pred) {
    if (pred->getType() == PredicateType::ASSUME ||
            pred->getType() == PredicateType::ENSURES ||
            pred->getType() == PredicateType::REQUIRES)
        return nullptr;
    else return pred;
}

}   // namespace borealis

#include "Util/unmacros.h"
