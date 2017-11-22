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

static config::BoolConfigEntry enableLogging("absint", "checker-logging");

namespace impl_ {

Checker::Checker(FactoryNest FN, DomainFactory* DF, State::Ptr state, std::vector<Domain::Ptr> args)
        : Transformer(FN),
          satisfied_(true),
          DF_(DF),
          state_(std::make_shared<State>(state->getVariables(), state->getConstants())),
          args_(std::move(args)) {}

Predicate::Ptr Checker::transformEqualityPredicate(EqualityPredicatePtr pred) {
    if (not satisfied()) return pred;
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

Term::Ptr Checker::transformArgumentTerm(ArgumentTermPtr term) {
    if (not satisfied()) return term;
    state_->addVariable(term, args_.at(term->getIdx()));
    return term;
}

Term::Ptr Checker::transformBinaryTerm(BinaryTermPtr term) {
    if (not satisfied()) return term;
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

Term::Ptr Checker::transformCmpTerm(CmpTermPtr term) {
    if (not satisfied()) return term;
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

Term::Ptr Checker::transformOpaqueBigIntConstantTerm(OpaqueBigIntConstantTermPtr term) {
    if (not satisfied()) return term;
    if (state_->find(term)) return term;
    auto intTy = llvm::cast<type::Integer>(term->getType().get());
    auto integer = DF_->toInteger(llvm::APInt(intTy->getBitsize(), term->getRepresentation(), 10));
    state_->addConstant(term, DF_->getInteger(integer));
    return term;
}

Term::Ptr Checker::transformOpaqueBoolConstantTerm(OpaqueBoolConstantTermPtr term) {
    if (not satisfied()) return term;
    if (state_->find(term)) return term;
    state_->addConstant(term, DF_->getBool(term->getValue()));
    return term;
}

Term::Ptr Checker::transformOpaqueFloatingConstantTerm(OpaqueFloatingConstantTermPtr term) {
    if (not satisfied()) return term;
    if (state_->find(term)) return term;
    state_->addConstant(term, DF_->getFloat(term->getValue()));
    return term;
}

Term::Ptr Checker::transformOpaqueIntConstantTerm(OpaqueIntConstantTermPtr term) {
    if (not satisfied()) return term;
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

Term::Ptr Checker::transformOpaqueInvalidPtrTerm(OpaqueInvalidPtrTermPtr term) {
    if (not satisfied()) return term;
    if (state_->find(term)) return term;
    state_->addVariable(term, DF_->getNullptr(term->getType()));
    return term;
}

Term::Ptr Checker::transformOpaqueNullPtrTerm(OpaqueNullPtrTermPtr term) {
    if (not satisfied()) return term;
    if (state_->find(term)) return term;
    state_->addVariable(term, DF_->getNullptr(term->getType()));
    return term;
}

Term::Ptr Checker::transformOpaqueStringConstantTerm(OpaqueStringConstantTermPtr term) {
    if (not satisfied()) return term;
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

Term::Ptr Checker::transformOpaqueUndefTerm(OpaqueUndefTermPtr term) {
    if (not satisfied()) return term;
    state_->addVariable(term, DF_->getTop(term->getType()));
    return term;
}

bool Checker::satisfied() const {
    return satisfied_;
}

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
            auto state = interpreter_->getStateMap().at(currentBasic_);
            auto args = pred->getArgs().map(LAM(a, state->find(a))).toVector();
            auto checker = impl_::Checker(FN, DF_, state, args);
            checker.transform(req);
            auto satisfied = checker.satisfied();
            defects_[di] |= (not satisfied);

            if (enableLogging.get(false)) {
                auto&& info = infos();
                info << "Checking: " << pred->toString() << endl;
                info << "Defect: " << di << endl;
                info << "Req: " << req << endl;
                info << "Args: " << endl;
                auto i = 0U;
                for (auto&& arg : function->getInstance()->args()) {
                    info << arg.getName().str() << " = " << args[i++] << endl;
                }
                info << "Satisfied: " << satisfied << endl;
                info << "Bug: " << (not satisfied) << endl;
                info << endl;
            }
        }
    }
    return pred;
}

void ContractChecker::checkEns() {
    auto di = DM_->getDefect(DefectType::ENS_01, &F_->back().back());
    auto ens = FM_->getEns(F_);

    if (not ens->isEmpty()) {
        auto state = interpreter_->getStateMap().at(currentBasic_);
        std::vector<Domain::Ptr> args;
        for (auto&& arg : F_->args()) args.push_back(state->find(FN.Term->getArgumentTerm(&arg)));

        auto checker = impl_::Checker(FN, DF_, state, args);
        checker.transform(ens);
        auto satisfied = checker.satisfied();
        defects_[di] |= (not satisfied);

        if (enableLogging.get(false)) {
            auto&& info = infos();
            info << "Checking: " << F_->getName().str() << " ensures" << endl;
            info << "Defect: " << di << endl;
            info << "Ens: " << ens << endl;
            info << "Args: " << endl;
            auto i = 0U;
            for (auto&& arg : F_->args()) {
                info << arg.getName().str() << " = " << args[i++] << endl;
            }
            auto retval = F_->getReturnType()->isPointerTy() ?
                          FN.Term->getReturnPtrTerm(F_) :
                          FN.Term->getReturnValueTerm(F_);
            info << retval << " = " << state->find(retval) << endl;
            info << "Satisfied: " << satisfied << endl;
            info << "Bug: " << (not satisfied) << endl;
            info << endl;
        }
    }
}

} // namespace ps
} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"
