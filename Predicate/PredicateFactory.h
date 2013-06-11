/*
 * PredicateFactory.h
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#ifndef PREDICATEFACTORY_H_
#define PREDICATEFACTORY_H_

#include <memory>

#include "Predicate/Predicate.def"
#include "Predicate/Predicate.h"

namespace borealis {

class PredicateFactory {

public:

    typedef std::unique_ptr<PredicateFactory> Ptr;

    Predicate::Ptr getLoadPredicate(
            Term::Ptr lhv,
            Term::Ptr loadTerm) {
        return getEqualityPredicate(lhv, loadTerm);
    }

    Predicate::Ptr getStorePredicate(
            Term::Ptr lhv,
            Term::Ptr rhv) {
        return Predicate::Ptr(
                new StorePredicate(lhv, rhv));
    }

    Predicate::Ptr getAllocaPredicate(
             Term::Ptr lhv,
             Term::Ptr numElements) {
        return Predicate::Ptr(
                new AllocaPredicate(lhv, numElements));
    }

    Predicate::Ptr getMallocPredicate(
                 Term::Ptr lhv,
                 Term::Ptr numElements) {
        return Predicate::Ptr(
                new MallocPredicate(lhv, numElements));
    }

    Predicate::Ptr getICmpPredicate(
            Term::Ptr lhv,
            Term::Ptr cmpTerm) {
        return getEqualityPredicate(lhv, cmpTerm);
    }

    Predicate::Ptr getBooleanPredicate(
            Term::Ptr v,
            Term::Ptr b) {
        return getEqualityPredicate(v, b, PredicateType::PATH);
    }

    Predicate::Ptr getDefaultSwitchCasePredicate(
            Term::Ptr cond,
            std::vector<Term::Ptr> cases) {
        return Predicate::Ptr(
            new DefaultSwitchCasePredicate(
                cond,
                cases,
                PredicateType::PATH)
        );
    }

    Predicate::Ptr getEqualityPredicate(
            Term::Ptr lhv,
            Term::Ptr rhv,
            PredicateType type = PredicateType::STATE) {
        return Predicate::Ptr(
                new EqualityPredicate(lhv, rhv, type));
    }

    Predicate::Ptr getInequalityPredicate(
            Term::Ptr lhv,
            Term::Ptr rhv,
            PredicateType type = PredicateType::STATE) {
        return Predicate::Ptr(
                new InequalityPredicate(lhv, rhv, type));
    }

    Predicate::Ptr getArithPredicate(
            Term::Ptr lhv,
            Term::Ptr arithTerm) {
        return getEqualityPredicate(lhv, arithTerm);
    }

    Predicate::Ptr getGlobalsPredicate(
            const std::vector<Term::Ptr>& globals) {
        return Predicate::Ptr(
                new GlobalsPredicate(globals));
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
