//
// Created by abdullin on 11/22/17.
//

#ifndef BOREALIS_QUERYCHECKER_H
#define BOREALIS_QUERYCHECKER_H

#include "Interpreter.h"
#include "Transformer.hpp"

namespace borealis {
namespace absint {
namespace ps {

class QueryChecker: public Transformer<QueryChecker>, logging::ObjectLevelLogging<QueryChecker> {
public:
    using Base = Transformer<QueryChecker>;
    using TermMap = std::unordered_map<Term::Ptr, Term::Ptr, TermHash, TermEquals>;

    QueryChecker(FactoryNest FN, const VariableFactory* vf, State::Ptr state);

    Predicate::Ptr transformEqualityPredicate(EqualityPredicatePtr pred);
    Predicate::Ptr transformInequalityPredicate(InequalityPredicatePtr pred);

    Term::Ptr transformArgumentTerm(ArgumentTermPtr term);
    Term::Ptr transformBinaryTerm(BinaryTermPtr term);
    Term::Ptr transformBoundTerm(BoundTermPtr term);
    Term::Ptr transformCastTerm(CastTermPtr term);
    Term::Ptr transformCmpTerm(CmpTermPtr term);
    Term::Ptr transformGepTerm(GepTermPtr term);
    Term::Ptr transformOpaqueBigIntConstantTerm(OpaqueBigIntConstantTermPtr term);
    Term::Ptr transformOpaqueBoolConstantTerm(OpaqueBoolConstantTermPtr term);
    Term::Ptr transformOpaqueFloatingConstantTerm(OpaqueFloatingConstantTermPtr term);
    Term::Ptr transformOpaqueIntConstantTerm(OpaqueIntConstantTermPtr term);
    Term::Ptr transformOpaqueInvalidPtrTerm(OpaqueInvalidPtrTermPtr term);
    Term::Ptr transformOpaqueNullPtrTerm(OpaqueNullPtrTermPtr term);
    Term::Ptr transformOpaqueStringConstantTerm(OpaqueStringConstantTermPtr term);
    Term::Ptr transformOpaqueUndefTerm(OpaqueUndefTermPtr term);
    Term::Ptr transformReadPropertyTerm(ReadPropertyTermPtr term);
    Term::Ptr transformValueTerm(ValueTermPtr term);

    bool satisfied() const;

private:
    bool satisfied_;
    const VariableFactory* vf_;
    State::Ptr state_;
};

} // namespace ps
} // namespace absint
} // namespace borealis

#endif //BOREALIS_QUERYCHECKER_H
