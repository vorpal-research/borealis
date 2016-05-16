//
// Created by belyaev on 5/12/16.
//

#include "State/Transformer/TermSizeCalculator.h"

namespace borealis {

TermSizeCalculator::TermSizeCalculator() : Base(FactoryNest{}) {}

size_t TermSizeCalculator::getTermSize() const {
    return termSize;
}

size_t TermSizeCalculator::getPredicateSize() const {
    return predicateSize;
}

Term::Ptr TermSizeCalculator::transformTerm(Term::Ptr term) {
    ++termSize;
    return Base::transformTerm(term);
}

Predicate::Ptr TermSizeCalculator::transformPredicate(Predicate::Ptr pred) {
    ++predicateSize;
    return Base::transformPredicate(pred);
}

} /* namespace borealis */
