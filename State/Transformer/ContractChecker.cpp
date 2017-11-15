//
// Created by abdullin on 11/15/17.
//

#include "ContractChecker.h"
#include "Interpreter/IR/Function.h"
#include "Interpreter/Domain/DomainFactory.h"

#include "Util/macros.h"

namespace borealis {
namespace absint {
namespace ps {

namespace impl_ {

class Checker : public Transformer<Checker> {
public:
    using Base = Transformer<Checker>;
    using TermMap = std::unordered_map<Term::Ptr, Term::Ptr, TermHash, TermEquals>;

    Checker(FactoryNest FN, DomainFactory* DF, State::Ptr state, std::vector<Term::Ptr> args)
            : Transformer(FN),
              satisfied_(true),
              DF_(DF),
              state_(std::make_shared<State>(state->getVariables(), state->getConstants())),
              args_(std::move(args)) {}

    Predicate::Ptr transformEqualityPredicate(EqualityPredicatePtr pred) {
        if (pred->getType() == PredicateType::REQUIRES || pred->getType() == PredicateType::ENSURES) {
            auto trueTerm = FN.Term->getTrueTerm();
            auto falseTerm = FN.Term->getFalseTerm();
            auto cond = state_->find(pred->getLhv());
            ASSERT(cond, "Unknown lhv in requires");

            if (pred->getRhv()->equals(trueTerm.get())) {
                auto boolean = llvm::dyn_cast<IntegerIntervalDomain>(cond.get());
                ASSERT(boolean && boolean->getWidth() == 1, "Non-bool in branch condition");
                satisfied_ &= boolean->isConstant(1);

            } else if (pred->getRhv()->equals(falseTerm.get())) {
                auto boolean = llvm::dyn_cast<IntegerIntervalDomain>(cond.get());
                ASSERT(boolean && boolean->getWidth() == 1, "Non-bool in branch condition");
                satisfied_ &= boolean->isConstant(0);

            } else {
                warns() << "Unknown req predicate: " << pred->toString() << endl;
            }

        }
        return pred;
    }

    Term::Ptr transformArgumentTerm(ArgumentTermPtr term) {
        state_->addVariable(term, state_->find(args_.at(term->getIdx())));
        return term;
    }

    Term::Ptr transformBinaryTerm(BinaryTermPtr term) {
        if (state_->find(term)) return term;
        auto lhv = state_->find(term->getLhv());
        auto rhv = state_->find(term->getRhv());
        ASSERT(lhv && rhv, "binop args of " + term->getName());

        Domain::Ptr result = nullptr;
        switch (term->getOpcode()) {
            case llvm::ArithType::SHL:
                result = lhv->shl(rhv);
                break;
            case llvm::ArithType::LSHR:
                result = lhv->lshr(rhv);
                break;
            case llvm::ArithType::ASHR:
                result = lhv->ashr(rhv);
                break;
            case llvm::ArithType::LAND:
            case llvm::ArithType::BAND:
                result = lhv->bAnd(rhv);
                break;
            case llvm::ArithType::LOR:
            case llvm::ArithType::BOR:
                result = lhv->bOr(rhv);
                break;
            case llvm::ArithType::XOR:
                result = lhv->bXor(rhv);
                break;
            case llvm::ArithType::IMPLIES:
                result = lhv->implies(rhv);
                break;
            default:
                UNREACHABLE("Unknown binary operator in req: " + term->getName());
        }

        ASSERT(result, "binop result " + term->getName());
        state_->addVariable(term, result);
        return term;
    }

    Term::Ptr transformCmpTerm(CmpTermPtr term) {
        if (state_->find(term)) return term;
        auto lhv = state_->find(term->getLhv());
        auto rhv = state_->find(term->getRhv());
        ASSERT(lhv && rhv, "cmp args of " + term->getName());

        bool isFloat = llvm::isa<type::Float>(term->getLhv()->getType().get());
        Domain::Ptr result;
        switch (term->getOpcode()) {
            case llvm::ConditionType::EQ:
                result = isFloat ?
                         lhv->fcmp(rhv, llvm::CmpInst::FCMP_UEQ) :
                         lhv->icmp(rhv, llvm::CmpInst::ICMP_EQ);
                break;
            case llvm::ConditionType::NEQ:
                result = isFloat ?
                         lhv->fcmp(rhv, llvm::CmpInst::FCMP_UNE) :
                         lhv->icmp(rhv, llvm::CmpInst::ICMP_NE);
                break;
            case llvm::ConditionType::GT:
                result = isFloat ?
                         lhv->fcmp(rhv, llvm::CmpInst::FCMP_UGT) :
                         lhv->icmp(rhv, llvm::CmpInst::ICMP_SGT);
                break;
            case llvm::ConditionType::GE:
                result = isFloat ?
                         lhv->fcmp(rhv, llvm::CmpInst::FCMP_UGE) :
                         lhv->icmp(rhv, llvm::CmpInst::ICMP_SGE);
                break;
            case llvm::ConditionType::LT:
                result = isFloat ?
                         lhv->fcmp(rhv, llvm::CmpInst::FCMP_ULT) :
                         lhv->icmp(rhv, llvm::CmpInst::ICMP_SLT);
                break;
            case llvm::ConditionType::LE:
                result = isFloat ?
                         lhv->fcmp(rhv, llvm::CmpInst::FCMP_ULE) :
                         lhv->icmp(rhv, llvm::CmpInst::ICMP_SLE);
                break;
            case llvm::ConditionType::UGT:
                result = isFloat ?
                         lhv->fcmp(rhv, llvm::CmpInst::FCMP_UGT) :
                         lhv->icmp(rhv, llvm::CmpInst::ICMP_UGT);
                break;
            case llvm::ConditionType::UGE:
                result = isFloat ?
                         lhv->fcmp(rhv, llvm::CmpInst::FCMP_UGE) :
                         lhv->icmp(rhv, llvm::CmpInst::ICMP_UGE);
                break;
            case llvm::ConditionType::ULT:
                result = isFloat ?
                         lhv->fcmp(rhv, llvm::CmpInst::FCMP_ULT) :
                         lhv->icmp(rhv, llvm::CmpInst::ICMP_ULT);
                break;
            case llvm::ConditionType::ULE:
                result = isFloat ?
                         lhv->fcmp(rhv, llvm::CmpInst::FCMP_ULE) :
                         lhv->icmp(rhv, llvm::CmpInst::ICMP_ULE);
                break;
            case llvm::ConditionType::TRUE:
                result = DF_->getBool(true);
                break;
            case llvm::ConditionType::FALSE:
                result = DF_->getBool(false);
                break;
            default:
                UNREACHABLE("Unknown cast: " + term->getName());
        }

        ASSERT(result, "cmp result " + term->getName());
        state_->addVariable(term, result);
        return term;
    }

    Term::Ptr transformOpaqueBigIntConstantTerm(OpaqueBigIntConstantTermPtr term) {
        if (state_->find(term)) return term;
        auto intTy = llvm::cast<type::Integer>(term->getType().get());
        auto integer = DF_->toInteger(llvm::APInt(intTy->getBitsize(), term->getRepresentation(), 10));
        state_->addConstant(term, DF_->getInteger(integer));
        return term;
    }

    Term::Ptr transformOpaqueBoolConstantTerm(OpaqueBoolConstantTermPtr term) {
        if (state_->find(term)) return term;
        state_->addConstant(term, DF_->getBool(term->getValue()));
        return term;
    }

    Term::Ptr transformOpaqueFloatingConstantTerm(OpaqueFloatingConstantTermPtr term) {
        if (state_->find(term)) return term;
        state_->addConstant(term, DF_->getFloat(term->getValue()));
        return term;
    }

    Term::Ptr transformOpaqueIntConstantTerm(OpaqueIntConstantTermPtr term) {
        if (state_->find(term)) return term;
        Integer::Ptr integer;
        if (auto intTy = llvm::dyn_cast<type::Integer>(term->getType().get())) {
            auto apInt = llvm::APInt(intTy->getBitsize(), term->getValue(),
                                     intTy->getSignedness() == llvm::Signedness::Signed);
            integer = DF_->toInteger(apInt);
            state_->addConstant(term, DF_->getInteger(integer));

        } else if (llvm::isa<type::Pointer>(term->getType().get())) {
            state_->addConstant(term, (term->getValue() == 0) ?
                                      DF_->getNullptr(term->getType()) :
                                      DF_->getTop(term->getType()));
        } else {
            warns() << "Unknown type in OpaqueIntConstant: " << TypeUtils::toString(*term->getType().get()) << endl;
            state_->addConstant(term, DF_->getIndex(term->getValue()));
        };
        return term;
    }

    Term::Ptr transformOpaqueInvalidPtrTerm(OpaqueInvalidPtrTermPtr term) {
        if (state_->find(term)) return term;
        state_->addVariable(term, DF_->getNullptr(term->getType()));
        return term;
    }

    Term::Ptr transformOpaqueNullPtrTerm(OpaqueNullPtrTermPtr term) {
        if (state_->find(term)) return term;
        state_->addVariable(term, DF_->getNullptr(term->getType()));
        return term;
    }

    Term::Ptr transformOpaqueStringConstantTerm(OpaqueStringConstantTermPtr term) {
        if (state_->find(term)) return term;
        if (auto array = llvm::dyn_cast<type::Array>(term->getType())) {
            std::vector<Domain::Ptr> elements;
            for (auto&& it : term->getValue()) {
                auto ch2i = DF_->toInteger(it, 8);
                elements.push_back(DF_->getInteger(ch2i));
            }
            state_->addConstant(term, DF_->getAggregate(array->getElement(), elements));
        } else {
            state_->addVariable(term, DF_->getTop(term->getType()));
        }
        return term;
    }

    Term::Ptr transformOpaqueUndefTerm(OpaqueUndefTermPtr term) {
        state_->addVariable(term, DF_->getTop(term->getType()));
        return term;
    }

    bool satisfied() const {
        return satisfied_;
    }

private:
    bool satisfied_;
    DomainFactory* DF_;
    State::Ptr state_;
    std::vector<Term::Ptr> args_;
};

} // namespace impl_

ContractChecker::ContractChecker(FactoryNest FN, llvm::Function* F,
                                 DefectManager* DM, FunctionManager* FM,
                                 DomainFactory* DF, Interpreter* interpreter)
        : Transformer(FN),
          ObjectLevelLogging("ps-interpreter"),
          F_(F),
          DM_(DM),
          FM_(FM),
          DF_(DF),
          interpreter_(interpreter) {}

void ContractChecker::apply() {
    checkEns();
    util::viewContainer(defects_)
            .filter(LAM(a, not a.second))
            .foreach([&](auto&& it) -> void { DM_->addNoAbsIntDefect(it.first); });
}

PredicateState::Ptr ContractChecker::transformBasic(Transformer::BasicPredicateStatePtr basic) {
    currentBasic_ = basic;
    return Base::transformBasic(basic);
}

Predicate::Ptr ContractChecker::transformCallPredicate(Transformer::CallPredicatePtr pred) {
    ir::Function::Ptr function;
    if (auto&& fname = llvm::dyn_cast<ValueTerm>(pred->getFunctionName().get())) {
        function = DF_->getGlobalVariableManager()->getFunctionByName(fname->getVName());
    } else {
        function = DF_->getGlobalVariableManager()->getFunctionByName(pred->getFunctionName()->getName());
    }

    if (function) {
        auto di = DefectInfo{DefectTypes.at(DefectType::REQ_01).type, pred->getLocation()};
        auto req = FM_->getReq(function->getInstance());

        if (not req->isEmpty()) {
            auto checker = impl_::Checker(FN, DF_,
                                          interpreter_->getStateMap().at(currentBasic_),
                                          pred->getArgs().toVector());
            checker.transform(req);
            auto satisfied = checker.satisfied();
            defects_[di] |= (not satisfied);
        }
    }
    return pred;
}

void ContractChecker::checkEns() {
    auto di = DM_->getDefect(DefectType::ENS_01, &F_->back().back());
    auto ens = FM_->getEns(F_);

    if (not ens->isEmpty()) {
        auto checker = impl_::Checker(FN, DF_,
                                      interpreter_->getStateMap().at(currentBasic_),
                                      {});
        checker.transform(ens);
        auto satisfied = checker.satisfied();
        defects_[di] |= (not satisfied);
    }
}

} // namespace ps
} // namespace absint
} // namespace checker

#include "Util/unmacros.h"
