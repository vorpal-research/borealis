//
// Created by abdullin on 10/20/17.
//

#ifndef BOREALIS_INTERPRETER_H
#define BOREALIS_INTERPRETER_H

#include "Interpreter/Domain/AbstractDomain.hpp"
#include "Interpreter/PredicateState/DomainStorage.hpp"
#include "Transformer.hpp"

namespace borealis {
namespace absint {
namespace ps {

class Interpreter : public Transformer<Interpreter>, public logging::ObjectLevelLogging<Interpreter> {
public:
    using Base = Transformer<Interpreter>;
    using State = DomainStorage;
    using Globals = std::unordered_map<std::string, AbstractDomain::Ptr>;
    using TermMap = std::unordered_map<Term::Ptr, Term::Ptr, TermHash, TermEquals>;
    using TermMapPtr = std::shared_ptr<TermMap>;

    Interpreter(FactoryNest FN, const VariableFactory* vf);
    Interpreter(FactoryNest FN, const VariableFactory* vf, State::Ptr input, TermMapPtr equalities);

    State::Ptr getState() const;
    const TermMap& getEqualities() const;

    PredicateState::Ptr transformChoice(PredicateStateChoicePtr choice);
    PredicateState::Ptr transformChain(PredicateStateChainPtr chain);
    /// predicates
    Predicate::Ptr transformAllocaPredicate(AllocaPredicatePtr pred);
    Predicate::Ptr transformCallPredicate(CallPredicatePtr pred);
    Predicate::Ptr transformDefaultSwitchCasePredicate(DefaultSwitchCasePredicatePtr pred);
    Predicate::Ptr transformEqualityPredicate(EqualityPredicatePtr pred);
    Predicate::Ptr transformGlobalsPredicate(GlobalsPredicatePtr pred);
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

    AbstractDomain::Ptr getDomain(Term::Ptr term) const;

    void interpretState(PredicateState::Ptr ps);
    bool isConditionSatisfied(Predicate::Ptr pred, State::Ptr state);

    const VariableFactory* vf_;
    State::Ptr input_;
    State::Ptr output_;
    TermMapPtr equalities_;
};

}   // namespace ps
}   // namespace absint
}   // namespace borealis


#endif //BOREALIS_INTERPRETER_H
