//
// Created by abdullin on 10/20/17.
//

#ifndef BOREALIS_INTERPRETER_H
#define BOREALIS_INTERPRETER_H

#include "Interpreter/PredicateState/PSState.h"
#include "Transformer.hpp"

namespace borealis {
namespace absint {

class PSInterpreter : public Transformer<PSInterpreter>, public logging::ObjectLevelLogging<PSInterpreter> {
public:
    using Base = Transformer<PSInterpreter>;
    using Globals = std::unordered_map<std::string, Domain::Ptr>;

    PSInterpreter(FactoryNest FN, DomainFactory* DF, const Globals& globals, PSState::Ptr state = std::make_shared<PSState>());

    PSState::Ptr getState() const;

    PredicateState::Ptr transformChoice(PredicateStateChoicePtr choice);
    PredicateState::Ptr transformChain(PredicateStateChainPtr chain);

    /// predicates
    Predicate::Ptr transformAllocaPredicate(AllocaPredicatePtr pred);
    Predicate::Ptr transformDefaultSwitchCasePredicate(DefaultSwitchCasePredicatePtr pred);
    Predicate::Ptr transformEqualityPredicate(EqualityPredicatePtr pred);
    Predicate::Ptr transformGlobalsPredicate(GlobalsPredicatePtr pred);
    Predicate::Ptr transformInequalityPredicate(InequalityPredicatePtr pred);
    Predicate::Ptr transformMallocPredicate(MallocPredicatePtr pred);
    Predicate::Ptr transformMarkPredicate(MarkPredicatePtr pred);
    Predicate::Ptr transformStorePredicate(StorePredicatePtr pred);
    /// terms
    Term::Ptr transformArgumentCountTerm(ArgumentCountTermPtr term);
    Term::Ptr transformArgumentTerm(ArgumentTermPtr term);
    Term::Ptr transformAxiomTerm(AxiomTermPtr term);
    Term::Ptr transformBinaryTerm(BinaryTermPtr term);
    Term::Ptr transformBoundTerm(BoundTermPtr term);
    Term::Ptr transformCastTerm(CastTermPtr term);
    Term::Ptr transformCmpTerm(CmpTermPtr term);
    Term::Ptr transformConstTerm(ConstTermPtr term);
    Term::Ptr transformFreeVarTerm(FreeVarTermPtr term);
    Term::Ptr transformGepTerm(GepTermPtr term);
    Term::Ptr transformLoadTerm(LoadTermPtr term);
    Term::Ptr transformOpaqueBigIntConstantTerm(OpaqueBigIntConstantTermPtr term);
    Term::Ptr transformOpaqueBoolConstantTerm(OpaqueBoolConstantTermPtr term);
    Term::Ptr transformOpaqueBuiltinTerm(OpaqueBuiltinTermPtr term);
    Term::Ptr transformOpaqueCallTerm(OpaqueCallTermPtr term);
    Term::Ptr transformOpaqueFloatingConstantTerm(OpaqueFloatingConstantTermPtr term);
    Term::Ptr transformOpaqueIndexingTerm(OpaqueIndexingTermPtr term);
    Term::Ptr transformOpaqueIntConstantTerm(OpaqueIntConstantTermPtr term);
    Term::Ptr transformOpaqueInvalidPtrTerm(OpaqueInvalidPtrTermPtr term);
    Term::Ptr transformOpaqueMemberAccessTerm(OpaqueMemberAccessTermPtr term);
    Term::Ptr transformOpaqueNamedConstantTerm(OpaqueNamedConstantTermPtr term);
    Term::Ptr transformOpaqueNullPtrTerm(OpaqueNullPtrTermPtr term);
    Term::Ptr transformOpaqueStringConstantTerm(OpaqueStringConstantTermPtr term);
    Term::Ptr transformOpaqueUndefTerm(OpaqueUndefTermPtr term);
    Term::Ptr transformOpaqueVarTerm(OpaqueVarTermPtr term);
    Term::Ptr transformSignTerm(SignTermPtr term);
    Term::Ptr transformTernaryTerm(TernaryTermPtr term);
    Term::Ptr transformUnaryTerm(UnaryTermPtr term);
    Term::Ptr transformValueTerm(ValueTermPtr term);
    Term::Ptr transformVarArgumentTerm(VarArgumentTermPtr term);

private:

    void interpretState(PredicateState::Ptr ps);

    FactoryNest FN_;
    DomainFactory* DF_;
    Globals globals_;
    PSState::Ptr state_;
};

}   // namespace absint
}   // namespace borealis


#endif //BOREALIS_INTERPRETER_H
