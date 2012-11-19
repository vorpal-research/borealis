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
            Term::Ptr lhv,
            Term::Ptr rhv) {
        return new LoadPredicate(std::move(lhv), std::move(rhv), slotTracker);
    }

    Predicate* getStorePredicate(
            Term::Ptr lhv,
            Term::Ptr rhv) {
        return new StorePredicate(std::move(lhv), std::move(rhv), slotTracker);
    }

    Predicate* getICmpPredicate(
            Term::Ptr lhv,
            Term::Ptr op1,
            Term::Ptr op2,
            int cond) {
        return new ICmpPredicate(std::move(lhv), std::move(op1), std::move(op2), cond, slotTracker);
    }

    Predicate* getPathBooleanPredicate(
            Term::Ptr v,
            bool b) {
        return new BooleanPredicate(PredicateType::PATH, std::move(v), b, slotTracker);
    }

    Predicate* getGEPPredicate(
            Term::Ptr lhv,
            Term::Ptr rhv,
            const std::vector< std::pair<const llvm::Value*, uint64_t> > shifts) {
        return new GEPPredicate(std::move(lhv), std::move(rhv), shifts, slotTracker);
    }

    Predicate* getEqualityPredicate(
            Term::Ptr lhv,
            Term::Ptr rhv) {
        return new EqualityPredicate(std::move(lhv), std::move(rhv), slotTracker);
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
