/*
 * PredicateFactory.h
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#ifndef PREDICATEFACTORY_H_
#define PREDICATEFACTORY_H_

#include <memory>

#include "Predicate/Predicate.h"

#include "Predicate/BooleanPredicate.h"
#include "Predicate/EqualityPredicate.h"
#include "Predicate/GEPPredicate.h"
#include "Predicate/ICmpPredicate.h"
#include "Predicate/LoadPredicate.h"
#include "Predicate/StorePredicate.h"

namespace borealis {

class PredicateFactory {

public:

    Predicate* getLoadPredicate(
            const llvm::Value* lhv,
            const llvm::Value* rhv) {
        return new LoadPredicate(lhv, rhv, slotTracker);
    }

    Predicate* getStorePredicate(
            const llvm::Value* lhv,
            const llvm::Value* rhv) {
        return new StorePredicate(lhv, rhv, slotTracker);
    }

    Predicate* getICmpPredicate(
            const llvm::Value* lhv,
            const llvm::Value* op1,
            const llvm::Value* op2,
            int cond) {
        return new ICmpPredicate(lhv, op1, op2, cond, slotTracker);
    }

    Predicate* getPathBooleanPredicate(
            const llvm::Value* v,
            bool b) {
        return new BooleanPredicate(PredicateType::PATH, v, b, slotTracker);
    }

    Predicate* getGEPPredicate(
            const llvm::Value* lhv,
            const llvm::Value* rhv,
            const std::vector< std::pair<const llvm::Value*, uint64_t> > shifts) {
        return new GEPPredicate(lhv, rhv, shifts, slotTracker);
    }

    Predicate* getEqualityPredicate(
            const llvm::Value* lhv,
            const llvm::Value* rhv) {
        return new EqualityPredicate(lhv, rhv, slotTracker);
    }

    static std::unique_ptr<PredicateFactory> get(SlotTracker* slotTracker) {
        return std::unique_ptr<PredicateFactory>(new PredicateFactory(slotTracker));
    }

private:

    SlotTracker* slotTracker;

    PredicateFactory(SlotTracker* slotTracker);

};

} /* namespace borealis */

#endif /* PREDICATEFACTORY_H_ */
