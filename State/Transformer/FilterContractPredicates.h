//
// Created by abdullin on 10/24/17.
//

#ifndef BOREALIS_FILTERCONTRACTPREDICATES_H
#define BOREALIS_FILTERCONTRACTPREDICATES_H

#include "Transformer.hpp"

namespace borealis {

class FilterContractPredicates : public Transformer<FilterContractPredicates> {
public:
    using Base = Transformer<FilterContractPredicates>;

    FilterContractPredicates(FactoryNest FN);
    PredicateState::Ptr transform(PredicateState::Ptr ps);
    Predicate::Ptr transformPredicate(Predicate::Ptr pred);
};

}   // namespace borealis

#endif //BOREALIS_FILTERCONTRACTPREDICATES_H
