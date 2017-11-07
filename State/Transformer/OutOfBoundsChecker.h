//
// Created by abdullin on 11/1/17.
//

#ifndef BOREALIS_OUTOFBOUNDSCHECKER_H
#define BOREALIS_OUTOFBOUNDSCHECKER_H

#include "Interpreter.h"
#include "Passes/Defect/DefectManager.h"
#include "Transformer.hpp"

namespace borealis {
namespace absint {
namespace ps {

class OutOfBoundsChecker: public Transformer<OutOfBoundsChecker>,
                          public logging::ObjectLevelLogging<OutOfBoundsChecker> {
public:
    using Base = Transformer<OutOfBoundsChecker>;

    OutOfBoundsChecker(FactoryNest FN, DefectManager* DM, DomainFactory* DF, Interpreter* interpreter);

    void apply();

    PredicateState::Ptr transformBasic(BasicPredicateStatePtr basic);
    Predicate::Ptr transformPredicate(Predicate::Ptr pred);
    Term::Ptr transformGepTerm(GepTermPtr term);

private:
    PredicateState::Ptr currentBasic_;
    const Locus* currentLocus_;
    DefectManager* DM_;
    DomainFactory* DF_;
    Interpreter* interpreter_;
    std::unordered_map<DefectInfo, bool> defects_;
};

}   // namespace ps
}   // namespace absint
}   // namespace borealis


#endif //BOREALIS_OUTOFBOUNDSCHECKER_H
