//
// Created by abdullin on 11/7/17.
//

#ifndef BOREALIS_PS_NULLDEREFERENCECHECKER_H
#define BOREALIS_PS_NULLDEREFERENCECHECKER_H

#include "Interpreter.h"
#include "Passes/Defect/DefectManager.h"
#include "Transformer.hpp"

namespace borealis {
namespace absint {
namespace ps {

class NullDereferenceChecker: public Transformer<NullDereferenceChecker>,
                          public logging::ObjectLevelLogging<NullDereferenceChecker> {
public:
    using Base = Transformer<NullDereferenceChecker>;

    NullDereferenceChecker(FactoryNest FN, DefectManager* DM, Interpreter* interpreter);

    void apply();

    PredicateState::Ptr transformBasic(BasicPredicateStatePtr basic);
    Predicate::Ptr transformPredicate(Predicate::Ptr pred);
    Predicate::Ptr transformStorePredicate(StorePredicatePtr pred);
    Term::Ptr transformGepTerm(GepTermPtr term);
    Term::Ptr transformLoadTerm(LoadTermPtr term);

private:

    void checkPtr(Term::Ptr memoryTerm, Term::Ptr ptr);

    PredicateState::Ptr currentBasic_;
    const Locus* currentLocus_;
    DefectManager* DM_;
    Interpreter* interpreter_;
    std::unordered_map<DefectInfo, bool> defects_;
};

}   // namespace ps
}   // namespace absint
}   // namespace borealis


#endif //BOREALIS_PS_NULLDEREFERENCECHECKER_H
