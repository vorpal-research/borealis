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

#include "Predicate/ArithPredicate.h"
#include "Predicate/EqualityPredicate.h"
#include "Predicate/GEPPredicate.h"
#include "Predicate/ICmpPredicate.h"
#include "Predicate/LoadPredicate.h"
#include "Predicate/StorePredicate.h"
#include "Predicate/AllocaPredicate.h"
#include "Predicate/MallocPredicate.h"
#include "Predicate/DefaultSwitchCasePredicate.h"

namespace borealis {

class PredicateFactory {

public:

    typedef std::unique_ptr<PredicateFactory> Ptr;

    Predicate::Ptr getLoadPredicate(
            Term::Ptr lhv,
            Term::Ptr rhv) {
        return Predicate::Ptr(
                new LoadPredicate(lhv, rhv, slotTracker));
    }

    Predicate::Ptr getStorePredicate(
            Term::Ptr lhv,
            Term::Ptr rhv) {
        return Predicate::Ptr(
                new StorePredicate(lhv, rhv, slotTracker));
    }

    Predicate::Ptr getAllocaPredicate(
             Term::Ptr lhv,
             Term::Ptr numElements) {
        return Predicate::Ptr(
                new AllocaPredicate(lhv, numElements, slotTracker));
    }

    Predicate::Ptr getMallocPredicate(
                 Term::Ptr lhv) {
        return Predicate::Ptr(
                new MallocPredicate(lhv, slotTracker));
    }

    Predicate::Ptr getICmpPredicate(
            Term::Ptr lhv,
            Term::Ptr op1,
            Term::Ptr op2,
            int cond) {
        return Predicate::Ptr(
                new ICmpPredicate(lhv, op1, op2, cond, slotTracker));
    }

    Predicate::Ptr getBooleanPredicate(
            Term::Ptr v,
            Term::Ptr b) {
        return Predicate::Ptr(
                new EqualityPredicate(
                        v,
                        b,
                        slotTracker,
                        PredicateType::PATH));
    }

    Predicate::Ptr getDefaultSwitchCasePredicate(
            Term::Ptr cond,
            std::vector<Term::Ptr> cases) {
        return Predicate::Ptr(
                new DefaultSwitchCasePredicate(
                        cond,
                        cases,
                        slotTracker,
                        PredicateType::PATH));
    }

    Predicate::Ptr getGEPPredicate(
            Term::Ptr lhv,
            Term::Ptr rhv,
            std::vector< std::pair< Term::Ptr, Term::Ptr > > shifts) {
        return Predicate::Ptr(
                new GEPPredicate(lhv, rhv, shifts, slotTracker));
    }

    Predicate::Ptr getEqualityPredicate(
            Term::Ptr lhv,
            Term::Ptr rhv) {
        return Predicate::Ptr(
                new EqualityPredicate(lhv, rhv, slotTracker));
    }

    Predicate::Ptr getArithPredicate(
            Term::Ptr lhv,
            Term::Ptr op1,
            Term::Ptr op2,
            llvm::ArithType opCode) {
        return Predicate::Ptr(
                new ArithPredicate(lhv, op1, op2, opCode, slotTracker));
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
