//
// Created by abdullin on 10/24/17.
//

#include "PropertyPredicateFilterer.h"

#include "Util/macros.h"

namespace borealis {

PropertyPredicateFilterer::PropertyPredicateFilterer(borealis::FactoryNest FN) : Transformer(FN) {}

PredicateState::Ptr PropertyPredicateFilterer::transform(PredicateState::Ptr ps) {
    return Base::transform(ps)
            ->filter(LAM(p, !!p))
            ->simplify();
}

Predicate::Ptr PropertyPredicateFilterer::transformPredicate(Predicate::Ptr pred) {
    if (llvm::isa<WritePropertyPredicate>(pred.get())) return nullptr;
    else {
        for (auto&& it : util::viewContainer(pred->getOperands())) {
            if (not util::viewContainer(Term::getFullTermSet(it)).map(LAM(a, llvm::dyn_cast<ReadPropertyTerm>(a.get())))
                    .filter().empty()) return nullptr;
        }
    }
    return pred;
}

}   // namespace borealis

#include "Util/unmacros.h"
