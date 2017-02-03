//
// Created by ice-phoenix on 5/26/15.
//

#include "ArrayBoundsCollector.h"

namespace borealis {

ArrayBoundsCollector::ArrayBoundsCollector(FactoryNest FN) : Base(FN) {}

Predicate::Ptr ArrayBoundsCollector::transformPredicate(Predicate::Ptr pred) {
    using namespace functional_hell::matchers;
    using namespace functional_hell::matchers::placeholders;

    if (auto&& m = $EqualityPredicate(_1, $GepTerm(_2, _)) >> pred) {
        if (llvm::isa<type::Pointer>(m->_2->getType())) {
            arrayBounds[m->_2].insert(m->_1);
        }
    }

    return pred;
}

const ArrayBoundsCollector::ArrayBounds& ArrayBoundsCollector::getArrayBounds() const {
    return arrayBounds;
}

}// namespace borealis
