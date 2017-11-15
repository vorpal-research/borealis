//
// Created by abdullin on 10/24/17.
//

#ifndef BOREALIS_FILTERCONTRACTPREDICATES_H
#define BOREALIS_FILTERCONTRACTPREDICATES_H

#include "Transformer.hpp"

namespace borealis {

class PropertyPredicateFilterer : public Transformer<PropertyPredicateFilterer> {
public:
    using Base = Transformer<PropertyPredicateFilterer>;

    PropertyPredicateFilterer(FactoryNest FN);
    PredicateState::Ptr transform(PredicateState::Ptr ps);
    Predicate::Ptr transformPredicate(Predicate::Ptr pred);
};

}   // namespace borealis

#endif //BOREALIS_FILTERCONTRACTPREDICATES_H
