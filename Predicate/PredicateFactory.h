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
#include "Predicate/AllocaPredicate.h"
#include "Predicate/MallocPredicate.h"
#include "Predicate/ArithPredicate.h"

namespace borealis {

class PredicateFactory {

public:

    typedef std::unique_ptr<PredicateFactory> Ptr;

    Predicate::Ptr getLoadPredicate(
            Term::Ptr lhv,
            Term::Ptr rhv) {
        return Predicate::Ptr(
                new LoadPredicate(std::move(lhv), std::move(rhv), slotTracker));
    }

    Predicate::Ptr getStorePredicate(
            Term::Ptr lhv,
            Term::Ptr rhv) {
        return Predicate::Ptr(
                new StorePredicate(std::move(lhv), std::move(rhv), slotTracker));
    }

    Predicate::Ptr getAllocaPredicate(
             Term::Ptr lhv,
             Term::Ptr numElements) {
        return Predicate::Ptr(
                new AllocaPredicate(std::move(lhv), std::move(numElements), slotTracker));
    }

    Predicate::Ptr getMallocPredicate(
                 Term::Ptr lhv) {
        return Predicate::Ptr(
                new MallocPredicate(std::move(lhv), slotTracker));
    }

    Predicate::Ptr getICmpPredicate(
            Term::Ptr lhv,
            Term::Ptr op1,
            Term::Ptr op2,
            int cond) {
        return Predicate::Ptr(
                new ICmpPredicate(std::move(lhv), std::move(op1), std::move(op2), cond, slotTracker));
    }

    Predicate::Ptr getPathBooleanPredicate(
            Term::Ptr v,
            bool b) {
        return Predicate::Ptr(
                new BooleanPredicate(PredicateType::PATH, std::move(v), b, slotTracker));
    }

    Predicate::Ptr getGEPPredicate(
            Term::Ptr lhv,
            Term::Ptr rhv,
            std::vector< std::pair<llvm::Value*, uint64_t> > shifts) {
        return Predicate::Ptr(
                new GEPPredicate(std::move(lhv), std::move(rhv), shifts, slotTracker));
    }

    Predicate::Ptr getEqualityPredicate(
            Term::Ptr lhv,
            Term::Ptr rhv) {
        return Predicate::Ptr(
                new EqualityPredicate(std::move(lhv), std::move(rhv), slotTracker));
    }

    Predicate::Ptr getArithPredicate(
            Term::Ptr lhv,
            Term::Ptr op1,
            Term::Ptr op2,
            llvm::ArithType opCode) {
        return Predicate::Ptr(
                new ArithPredicate(std::move(lhv), std::move(op1), std::move(op2), opCode, slotTracker));
    }

    static Ptr get(SlotTracker* slotTracker) {
        return Ptr(new PredicateFactory(slotTracker));
    }

private:

    SlotTracker* slotTracker;

    PredicateFactory(SlotTracker* slotTracker);

};

} /* namespace borealis */

#endif /* PREDICATEFACTORY_H_ */
