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

    typedef std::shared_ptr<PredicateFactory> Ptr;

    Predicate::Ptr getLoadPredicate(
            Term::Ptr lhv,
            Term::Ptr loadTerm,
            const Locus& loc = Locus()) {
        return getEqualityPredicate(lhv, loadTerm, loc);
    }

    Predicate::Ptr getStorePredicate(
            Term::Ptr lhv,
            Term::Ptr rhv,
            const Locus& loc = Locus(),
            PredicateType type = PredicateType::STATE) {
        return Predicate::Ptr(
                new StorePredicate(lhv, rhv, loc, type));
    }

    Predicate::Ptr getCallPredicate(
            Term::Ptr lhv,
            Term::Ptr function,
            const std::vector<Term::Ptr>& args,
            const Locus& loc = Locus()) {
        return Predicate::Ptr(new CallPredicate(lhv, function, args, loc));
    }

    Predicate::Ptr getWritePropertyPredicate(
            Term::Ptr propName,
            Term::Ptr lhv,
            Term::Ptr rhv,
            const Locus& loc = Locus()) {

        return Predicate::Ptr(
                new WritePropertyPredicate(propName, lhv, rhv, loc));
    }

    Predicate::Ptr getWriteBoundPredicate(
            Term::Ptr lhv,
            Term::Ptr rhv,
            const Locus& loc = Locus()) {
        return Predicate::Ptr(new WriteBoundPredicate(lhv, rhv, loc));
    }

    Predicate::Ptr getAllocaPredicate(
             Term::Ptr lhv,
             Term::Ptr numElements,
             Term::Ptr origNumElements,
             const Locus& loc = Locus()) {
        return Predicate::Ptr(
                new AllocaPredicate(lhv, numElements, origNumElements, loc));
    }

    Predicate::Ptr getMallocPredicate(
                 Term::Ptr lhv,
                 Term::Ptr numElements,
                 Term::Ptr origNumElements,
                 const Locus& loc = Locus()) {
        return Predicate::Ptr(
                new MallocPredicate(lhv, numElements, origNumElements, loc));
    }



    Predicate::Ptr getBooleanPredicate(
            Term::Ptr v,
            Term::Ptr b,
            const Locus& loc = Locus()) {
        return getEqualityPredicate(v, b, loc, PredicateType::PATH);
    }

    Predicate::Ptr getDefaultSwitchCasePredicate(
            Term::Ptr cond,
            std::vector<Term::Ptr> cases,
            const Locus& loc = Locus()) {
        return Predicate::Ptr(
            new DefaultSwitchCasePredicate(
                cond,
                cases,
                loc,
                PredicateType::PATH)
        );
    }



    Predicate::Ptr getEqualityPredicate(
            Term::Ptr lhv,
            Term::Ptr rhv,
            const Locus& loc = Locus(),
            PredicateType type = PredicateType::STATE) {
        return Predicate::Ptr(
                new EqualityPredicate(lhv, rhv, loc, type));
    }

    Predicate::Ptr getInequalityPredicate(
            Term::Ptr lhv,
            Term::Ptr rhv,
            const Locus& loc = Locus(),
            PredicateType type = PredicateType::STATE) {
        return Predicate::Ptr(
                new InequalityPredicate(lhv, rhv, loc, type));
    }



    Predicate::Ptr getGlobalsPredicate(
            const std::vector<Term::Ptr>& globals,
            const Locus& loc = Locus()) {
        return Predicate::Ptr(
                new GlobalsPredicate(globals, loc));
    }

    Predicate::Ptr getSeqDataPredicate(
            Term::Ptr base,
            const std::vector<Term::Ptr>& data,
            const Locus& loc = Locus()) {
        return Predicate::Ptr(
                new SeqDataPredicate(base, data, loc));
    }

    Predicate::Ptr getSeqDataZeroPredicate(
            Term::Ptr base,
            size_t size,
            const Locus& loc = Locus()) {
        return Predicate::Ptr(
                new SeqDataZeroPredicate(base, size, loc));
    }

    Predicate::Ptr getMarkPredicate(Term::Ptr id, const Locus& loc = Locus()) {
        return Predicate::Ptr(new MarkPredicate(id, loc));
    }

    static PredicateFactory::Ptr get() {
        static PredicateFactory::Ptr instance(new PredicateFactory());
        return instance;
    }

private:

    PredicateFactory() {};

};

} /* namespace borealis */

#endif /* PREDICATEFACTORY_H_ */
