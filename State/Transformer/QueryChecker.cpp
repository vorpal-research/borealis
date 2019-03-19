//
// Created by abdullin on 11/22/17.
//

#include "Interpreter/Domain/AbstractFactory.hpp"
#include "Interpreter/Domain/Numerical/Interval.hpp"
#include "Interpreter/Domain/Memory/PointerDomain.hpp"
#include "Interpreter/Domain/VariableFactory.hpp"
#include "QueryChecker.h"

#include "Util/macros.h"

namespace borealis {
namespace absint {
namespace ps {

QueryChecker::QueryChecker(FactoryNest FN, const VariableFactory* vf, State::Ptr state)
        : Transformer(FN),
          ObjectLevelLogging("ps-interpreter"),
          satisfied_(true),
          vf_(vf),
          state_(state) {}

bool QueryChecker::satisfied() const {
    return satisfied_;
}

Predicate::Ptr QueryChecker::transformEqualityPredicate(EqualityPredicatePtr pred) {
    if (not satisfied()) return pred;

    auto trueTerm = FN.Term->getTrueTerm();
    auto falseTerm = FN.Term->getFalseTerm();
    auto cond = state_->get(pred->getLhv());
    ASSERT(cond, "Unknown lhv in query");

    if (pred->getRhv()->equals(trueTerm.get())) {
        auto boolean = llvm::dyn_cast<AbstractFactory::BoolT>(cond.get());
        ASSERTC(boolean);
        satisfied_ &= boolean->isConstant(1);

    } else if (pred->getRhv()->equals(falseTerm.get())) {
        auto boolean = llvm::dyn_cast<AbstractFactory::BoolT>(cond.get());
        ASSERTC(boolean);
        satisfied_ &= boolean->isConstant(0);

    } else {
        warns() << "Unknown query predicate: " << pred->toString() << endl;
    }

    return pred;
}

Predicate::Ptr QueryChecker::transformInequalityPredicate(InequalityPredicatePtr pred) {
    if (not satisfied()) return pred;

    auto trueTerm = FN.Term->getTrueTerm();
    auto falseTerm = FN.Term->getFalseTerm();
    auto lhv = state_->get(pred->getLhv());
    auto rhv = state_->get(pred->getRhv());
    ASSERT(lhv && rhv, "Unknown predicate in query:" + pred->toString());

    auto res = lhv->apply(llvm::ConditionType::NEQ, rhv);
    auto boolean = llvm::dyn_cast<AbstractFactory::BoolT>(res.get());
    ASSERTC(boolean);
    satisfied_ &= boolean->isConstant(1);

    return pred;
}

Term::Ptr QueryChecker::transformArgumentTerm(ArgumentTermPtr term) {
    auto&& domain = state_->get(term);
    state_->assign(term, domain ? domain : vf_->top(term->getType()));
    return term;
}

Term::Ptr QueryChecker::transformBinaryTerm(BinaryTermPtr term) {
    if (not satisfied()) return term;
    if (state_->get(term)) return term;
    auto lhv = state_->get(term->getLhv());
    auto rhv = state_->get(term->getRhv());
    ASSERT(lhv && rhv, "binop args of " + term->getName());

    AbstractDomain::Ptr result = lhv->apply(term->getOpcode(), rhv);
    ASSERT(result, "binop result " + term->getName());
    state_->assign(term, result);
    return term;
}

Term::Ptr QueryChecker::transformBoundTerm(BoundTermPtr term) {
    if (not satisfied()) return term;
    auto ptr = state_->get(term->getRhv());
    ASSERT(ptr, "bound term arg: " + term->getName());

    auto ptrDom = llvm::dyn_cast<AbstractFactory::PointerT>(ptr.get());
    ASSERT(ptrDom, "bound term arg is not a pointer" + ptr->toString());
    state_->assign(term, ptrDom->bound());
    return term;
}

Term::Ptr QueryChecker::transformCastTerm(CastTermPtr term) {
    if (not satisfied()) return term;
    auto cast = state_->get(term->getRhv());
    ASSERT(cast, "cast arg of " + term->getName());
    auto fromTy = term->getRhv()->getType();
    auto toTy = term->getType();

    CastOperator op;
    if (auto m = util::match_tuple<type::Integer, type::Integer>::doit(fromTy, toTy)) {
        auto fromBitsize = m->get<0>()->getBitsize();
        auto toBitsize = m->get<1>()->getBitsize();
        if (toBitsize < fromBitsize) op = TRUNC;
        else op = term->isSignExtend() ?
                  SEXT :
                  EXT;

    } else if (util::match_tuple<type::Float, type::Float>::doit(fromTy, toTy)) {
        op = TRUNC;
    } else if (util::match_tuple<type::Float, type::Integer>::doit(fromTy, toTy)) {
        op = FPTOI;
    } else if (util::match_tuple<type::Integer, type::Float>::doit(fromTy, toTy)) {
        op = ITOFP;
    } else if (util::match_tuple<type::Pointer, type::Integer>::doit(fromTy, toTy)) {
        op = PTRTOI;
    } else if (util::match_tuple<type::Integer, type::Pointer>::doit(fromTy, toTy)) {
        op = ITOPTR;
    } else if (util::match_tuple<type::Pointer, type::Pointer>::doit(fromTy, toTy)) {
        op = BITCAST;
    } else {
        UNREACHABLE("Unknown cast: " + term->getName());
    }

    auto&& result = vf_->af()->cast(op, term->getType(), state_->get(term->getRhv()));

    ASSERT(result, "cast result " + term->getName());
    state_->assign(term, result);
    return term;
}

Term::Ptr QueryChecker::transformCmpTerm(CmpTermPtr term) {
    if (not satisfied()) return term;
    if (state_->get(term)) return term;
    auto lhv = state_->get(term->getLhv());
    auto rhv = state_->get(term->getRhv());
    ASSERT(lhv && rhv, "cmp args of " + term->getName());

    AbstractDomain::Ptr result = lhv->apply(term->getOpcode(), rhv);
    ASSERT(result, "binop result " + term->getName());
    state_->assign(term, result);
    return term;
}

Term::Ptr QueryChecker::transformOpaqueBigIntConstantTerm(OpaqueBigIntConstantTermPtr term) {
    return term;
}

Term::Ptr QueryChecker::transformOpaqueBoolConstantTerm(OpaqueBoolConstantTermPtr term) {
    return term;
}

Term::Ptr QueryChecker::transformOpaqueFloatingConstantTerm(OpaqueFloatingConstantTermPtr term) {
    return term;
}

Term::Ptr QueryChecker::transformOpaqueIntConstantTerm(OpaqueIntConstantTermPtr term) {
    return term;
}

Term::Ptr QueryChecker::transformOpaqueInvalidPtrTerm(OpaqueInvalidPtrTermPtr term) {
    return term;
}

Term::Ptr QueryChecker::transformOpaqueNullPtrTerm(OpaqueNullPtrTermPtr term) {
    return term;
}

Term::Ptr QueryChecker::transformOpaqueStringConstantTerm(OpaqueStringConstantTermPtr term) {
    return term;
}

Term::Ptr QueryChecker::transformGepTerm(GepTermPtr term) {
    if (not satisfied()) return term;
    state_->gep(term, term->getBase(), term->getShifts().toVector());
    return term;
}

Term::Ptr QueryChecker::transformOpaqueUndefTerm(OpaqueUndefTermPtr term) {
    if (not satisfied()) return term;
    state_->assign(term, vf_->top(term->getType()));
    return term;
}

Term::Ptr QueryChecker::transformReadPropertyTerm(Transformer::ReadPropertyTermPtr term) {
    satisfied_ = false;
    return term;
}

Term::Ptr QueryChecker::transformValueTerm(Transformer::ValueTermPtr term) {
    if (state_->get(term)) return term;
    if (term->isGlobal()) {
        auto domain = vf_->findGlobal(term->getVName());
        ASSERT(domain, "Unknown global variable: " + term->getName());
        state_->assign(term, domain);

    } else {
        state_->assign(term, vf_->top(term->getType()));
    }
    return term;
}

} // namespace ps
} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"