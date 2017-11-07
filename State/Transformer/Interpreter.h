//
// Created by abdullin on 10/20/17.
//

#ifndef BOREALIS_INTERPRETER_H
#define BOREALIS_INTERPRETER_H

#include "Interpreter/PredicateState/State.h"
#include "Transformer.hpp"

namespace borealis {
namespace absint {
namespace ps {

class Interpreter : public Transformer<Interpreter>, public logging::ObjectLevelLogging<Interpreter> {
public:
    using Base = Transformer<Interpreter>;
    using Globals = std::unordered_map<std::string, Domain::Ptr>;
    using TermMap = std::unordered_map<Term::Ptr, Term::Ptr, TermHash, TermEquals>;
    using StateMap = std::map<PredicateState::Ptr, State::Ptr>;

    Interpreter(FactoryNest FN, DomainFactory* DF,
                State::Ptr state = std::make_shared<State>(),
                const TermMap& equalities = TermMap());

    State::Ptr getState() const;
    const TermMap& getEqualities() const;
    const StateMap& getStateMap() const;

    PredicateState::Ptr transformBasic(BasicPredicateStatePtr basic);
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
    bool isConditionSatisfied(Predicate::Ptr pred, State::Ptr state);

    DomainFactory* DF_;
    State::Ptr state_;
    TermMap equalities_;
    StateMap states_;
};

}   // namespace ps
}   // namespace absint
}   // namespace borealis


#endif //BOREALIS_INTERPRETER_H
