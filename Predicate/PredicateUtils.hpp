#ifndef PREDICATE_UTILS_HPP
#define PREDICATE_UTILS_HPP

#include "Predicate/PredicateFactory.h"

namespace borealis {

struct PredicateUtils {
    static Term::Ptr getReceiver(Predicate::Ptr p) {
        if(p->getType() != PredicateType::STATE && p->getType() != PredicateType::INVARIANT) return nullptr;
        if(llvm::is_one_of<
            AllocaPredicate,
            MallocPredicate,
            EqualityPredicate,
            SeqDataPredicate,
            SeqDataZeroPredicate,
            StorePredicate,
            WriteBoundPredicate>(p)) {
            return p->getOperands()[0];
        }
        if(auto wb = llvm::dyn_cast<WritePropertyPredicate>(p)) {
            return wb->getLhv();
        }
        if(auto g = llvm::dyn_cast<GlobalsPredicate>(p)) {
            return g->getOperands().size() == 1 ? g->getOperands()[0] : nullptr;
        }

        return nullptr;
    }
};

} /* namespace borealis */

#endif // PREDICATE_UTILS_HPP
