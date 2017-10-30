//
// Created by abdullin on 10/20/17.
//

#include "PSInterpreter.h"
#include "Interpreter/Domain/DomainFactory.h"

#include "Util/macros.h"

namespace borealis {
namespace absint {

PSInterpreter::PSInterpreter(FactoryNest FN, DomainFactory* DF, PSState::Ptr state)
        : Transformer(FN),
          ObjectLevelLogging("ps-interpreter"),
          FN_(FN),
          DF_(DF),
          state_(state) {}

PSState::Ptr PSInterpreter::getState() const {
    return state_;
}

PredicateState::Ptr PSInterpreter::transformChoice(PredicateStateChoicePtr choice) {
    for (auto&& ch : choice->getChoices()) {
        interpretState(ch);
    }
    return choice;
}

PredicateState::Ptr PSInterpreter::transformChain(PredicateStateChainPtr chain) {
    interpretState(chain->getBase());
    interpretState(chain->getCurr());
    return chain;
}

void PSInterpreter::interpretState(PredicateState::Ptr ps) {
    auto interpreter = PSInterpreter(FN_, DF_, state_);
    interpreter.transform(ps);
    state_->merge(interpreter.getState());
}

Predicate::Ptr PSInterpreter::transformAllocaPredicate(AllocaPredicatePtr pred) {
    auto size = state_->find(llvm::cast<BinaryTerm>(pred->getOrigNumElems().get())->getRhv());
    auto&& integer = llvm::dyn_cast<IntegerIntervalDomain>(size.get());
    ASSERT(integer, "Non-integer domain in memory allocation");

    Domain::Ptr domain;
    auto predType = pred->getLhv()->getType();
    auto elemType = llvm::cast<type::Pointer>(predType.get())->getPointed();

    if (integer->ub()->isMax()) {
        domain = DF_->getTop(predType);
    } else {
        auto&& arrayType = FN.Type->getArray(elemType, integer->ub()->getRawValue());
        auto&& ptrType = FN.Type->getPointer(arrayType, 0);
        domain = DF_->allocate(ptrType);
    }

    ASSERT(domain, "alloca result");
    state_->addTerm(pred->getLhv(), domain);
    return pred;
}

Predicate::Ptr PSInterpreter::transformDefaultSwitchCasePredicate(DefaultSwitchCasePredicatePtr pred) {
    auto result = DF_->getBottom(pred->getCond()->getType());
    for (auto&& cs : pred->getCases()) result = result->join(state_->find(cs));
    state_->addTerm(pred->getCond(), result);
    return pred;
}

Predicate::Ptr PSInterpreter::transformEqualityPredicate(EqualityPredicatePtr pred) {
    auto rhv = state_->find(pred->getRhv());
    ASSERT(rhv, "Equality rhv: " + pred->toString());
    state_->addTerm(pred->getLhv(), rhv);
    return pred;
}

Predicate::Ptr PSInterpreter::transformGlobalsPredicate(GlobalsPredicatePtr pred) {
    for (auto&& it : pred->getGlobals()) {
        state_->addTerm(it, DF_->getTop(it->getType()));
    }
    return pred;
}

Predicate::Ptr PSInterpreter::transformInequalityPredicate(InequalityPredicatePtr pred) {
    errs() << "Inequality pred: " << pred->toString() << endl;
    state_->addTerm(pred->getLhv(), DF_->getTop(pred->getLhv()->getType()));
    return pred;
}

Predicate::Ptr PSInterpreter::transformMallocPredicate(MallocPredicatePtr pred) {
    auto size = state_->find(llvm::cast<BinaryTerm>(pred->getOrigNumElems().get())->getRhv());
    auto&& integer = llvm::dyn_cast<IntegerIntervalDomain>(size.get());
    ASSERT(integer, "Non-integer domain in memory allocation");

    Domain::Ptr domain;
    auto predType = pred->getLhv()->getType();
    auto elemType = llvm::cast<type::Pointer>(predType.get())->getPointed();

    if (integer->ub()->isMax()) {
        domain = DF_->getTop(predType);
    } else {
        auto&& arrayType = FN.Type->getArray(elemType, integer->ub()->getRawValue());
        auto&& ptrType = FN.Type->getPointer(arrayType, 0);
        domain = DF_->allocate(ptrType);
    }

    ASSERT(domain, "malloc result");
    state_->addTerm(pred->getLhv(), domain);
    return pred;
}

Predicate::Ptr PSInterpreter::transformMarkPredicate(MarkPredicatePtr pred) {
    errs() << "Mark pred: " << pred->toString() << endl;
    state_->addTerm(pred->getId(), DF_->getTop(pred->getId()->getType()));
    return pred;
}

Predicate::Ptr PSInterpreter::transformStorePredicate(StorePredicatePtr pred) {
    auto ptr = state_->find(pred->getLhv());
    auto storeVal = state_->find(pred->getRhv());
    ASSERT(ptr && storeVal, "store args of " + pred->toString());

    ptr->store(storeVal, DF_->getIndex(0));
    return pred;
}

Term::Ptr PSInterpreter::transformArgumentCountTerm(ArgumentCountTermPtr term) {
    errs() << "Unknown ArgumentCountTerm: " << term->getName() << endl;
    return term;
}

Term::Ptr PSInterpreter::transformArgumentTerm(ArgumentTermPtr term) {
    if (state_->find(term)) return term;
    state_->addTerm(term, DF_->getTop(term->getType()));
    return term;
}

Term::Ptr PSInterpreter::transformAxiomTerm(AxiomTermPtr term) {
    errs() << "Unknown AxiomTerm: " << term->getName() << endl;
    return term;
}

Term::Ptr PSInterpreter::transformBinaryTerm(BinaryTermPtr term) {
    if (state_->find(term)) return term;
    auto lhv = state_->find(term->getLhv());
    auto rhv = state_->find(term->getRhv());
    ASSERT(lhv && rhv, "binop args of " + term->getName());

    auto isFloat = llvm::isa<type::Float>(term->getType().get());

    Domain::Ptr result = nullptr;
    switch (term->getOpcode()) {
        case llvm::ArithType::ADD:
            result = isFloat ?
                     lhv->fadd(rhv) :
                     lhv->add(rhv);
            break;
        case llvm::ArithType::SUB:
            result = isFloat ?
                     lhv->fsub(rhv) :
                     lhv->sub(rhv);
            break;
        case llvm::ArithType::MUL:
            result = isFloat ?
                     lhv->fmul(rhv) :
                     lhv->mul(rhv);
            break;
        case llvm::ArithType::DIV:
            if (isFloat) result = lhv->fdiv(rhv);
            else if (auto intty = llvm::dyn_cast<type::Integer>(term->getType().get())) {
                result = (intty->getSignedness() == llvm::Signedness::Signed) ?
                         lhv->sdiv(rhv) :
                         lhv->udiv(rhv);
            }
            break;
        case llvm::ArithType::REM:
            if (isFloat) result = lhv->frem(rhv);
            else if (auto intty = llvm::dyn_cast<type::Integer>(term->getType().get())) {
                result = (intty->getSignedness() == llvm::Signedness::Signed) ?
                         lhv->srem(rhv) :
                         lhv->urem(rhv);
            }
            break;
        case llvm::ArithType::SHL:    result = lhv->shl(rhv); break;
        case llvm::ArithType::LSHR:   result = lhv->lshr(rhv); break;
        case llvm::ArithType::ASHR:   result = lhv->ashr(rhv); break;
        case llvm::ArithType::LAND:
        case llvm::ArithType::BAND:
            result = lhv->bAnd(rhv); break;
        case llvm::ArithType::LOR:
        case llvm::ArithType::BOR:
            result = lhv->bOr(rhv); break;
        case llvm::ArithType::XOR:    result = lhv->bXor(rhv); break;
        case llvm::ArithType::IMPLIES: result = lhv->implies(rhv); break;
        default:
            UNREACHABLE("Unknown binary operator: " + term->getName());
    }

    ASSERT(result, "binop result " + term->getName());
    state_->addTerm(term, result);
    return term;
}

Term::Ptr PSInterpreter::transformBoundTerm(BoundTermPtr term) {
    auto ptr = state_->find(term->getRhv());
    ASSERT(ptr, "bound term arg: " + term->getName());
    auto ptrDom = llvm::dyn_cast<PointerDomain>(ptr.get());
    ASSERT(ptrDom, "bound term arg is not a pointer" + ptr->toString());
    state_->addTerm(term, ptrDom->getBound());
    return term;
}

Term::Ptr PSInterpreter::transformCastTerm(CastTermPtr term) {
    if (state_->find(term)) return term;
    auto cast = state_->find(term->getRhv());
    ASSERT(cast, "cast arg of " + term->getName());
    auto fromTy = term->getRhv()->getType();
    auto toTy = term->getType();

    Domain::Ptr result;

    if (auto m = util::match_tuple<type::Integer, type::Integer>::doit(fromTy, toTy)) {
        auto fromBitsize = m->get<0>()->getBitsize();
        auto toBitsize = m->get<1>()->getBitsize();
        if (toBitsize < fromBitsize) result = cast->trunc(toTy);
        else if (toBitsize == fromBitsize) result = cast;
        else result = term->isSignExtend() ?
                     cast->sext(toTy) :
                     cast->zext(toTy);

    } else if (auto match = util::match_tuple<type::Float, type::Float>::doit(fromTy, toTy)) {
        result = cast;
    } else if (auto match = util::match_tuple<type::Float, type::Integer>::doit(fromTy, toTy)) {
        result = (match->get<1>()->getSignedness() == llvm::Signedness::Signed) ?
                 cast->fptosi(toTy) :
                 cast->fptoui(toTy);
    } else if (auto match = util::match_tuple<type::Integer, type::Float>::doit(fromTy, toTy)) {
        result = (match->get<0>()->getSignedness() == llvm::Signedness::Signed) ?
                 cast->sitofp(toTy) :
                 cast->uitofp(toTy);
    } else if (auto match = util::match_tuple<type::Pointer, type::Integer>::doit(fromTy, toTy)) {
        result = cast->ptrtoint(toTy);
    } else if (auto match = util::match_tuple<type::Integer, type::Pointer>::doit(fromTy, toTy)) {
        result = cast->inttoptr(toTy);
    } else if (auto match = util::match_tuple<type::Pointer, type::Pointer>::doit(fromTy, toTy)) {
        result = cast->bitcast(toTy);
    } else {
        UNREACHABLE("Unknown cast: " + term->getName());
    }

    ASSERT(result, "cast result " + term->getName());
    state_->addTerm(term, result);
    return term;
}

Term::Ptr PSInterpreter::transformCmpTerm(CmpTermPtr term) {
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
    state_->addTerm(term, result);
    return term;
}

Term::Ptr PSInterpreter::transformConstTerm(ConstTermPtr term) {
    errs() << "Unknown ConstTerm: " << term->getName() << endl;
    return term;
}

Term::Ptr PSInterpreter::transformFreeVarTerm(FreeVarTermPtr term) {
    errs() << "Unknown FreeVarTerm: " << term->getName() << endl;
    state_->addTerm(term, DF_->getTop(term->getType()));
    return term;
}

Term::Ptr PSInterpreter::transformGepTerm(GepTermPtr term) {
    if (state_->find(term)) return term;
    auto ptr = state_->find(term->getBase());
    ASSERT(ptr, "gep pointer of " + term->getName());

    std::vector<Domain::Ptr> shifts;
    for (auto&& it : term->getShifts()) {
        auto shift = state_->find(it);

        auto&& integer = llvm::dyn_cast<IntegerIntervalDomain>(shift.get());
        ASSERT(integer, "Non-integer domain in gep shift");

        if (integer->getWidth() < DF_->defaultSize) {
            shifts.emplace_back(shift->zext(FN.Type->getInteger(DF_->defaultSize)));
        } else if (integer->getWidth() > DF_->defaultSize) {
            shifts.emplace_back(shift->trunc(FN.Type->getInteger(DF_->defaultSize)));
        } else {
            shifts.emplace_back(shift);
        }
    }
    auto elementType = llvm::cast<type::Pointer>(term->getType().get())->getPointed();
    auto result = ptr->gep(elementType, shifts);

    ASSERT(result, "gep result " + term->getName());
    state_->addTerm(term, result);
    return term;
}

Term::Ptr PSInterpreter::transformLoadTerm(LoadTermPtr term) {
    if (state_->find(term)) return term;
    auto ptr = state_->find(term->getRhv());
    ASSERT(ptr, "load pointer of " + term->getName());

    auto result = ptr->load(term->getType(), DF_->getIndex(0));
    ASSERT(result, "load result " + term->getName());
    state_->addTerm(term, result);
    return term;
}

Term::Ptr PSInterpreter::transformOpaqueBigIntConstantTerm(OpaqueBigIntConstantTermPtr term) {
    if (state_->find(term)) return term;
    auto intTy = llvm::cast<type::Integer>(term->getType().get());
    auto integer = DF_->toInteger(llvm::APInt(intTy->getBitsize(), term->getRepresentation(), 10));
    state_->addConstant(term, DF_->getInteger(integer));
    return term;
}

Term::Ptr PSInterpreter::transformOpaqueBoolConstantTerm(OpaqueBoolConstantTermPtr term) {
    if (state_->find(term)) return term;
    state_->addConstant(term, DF_->getBool(term->getValue()));
    return term;
}

Term::Ptr PSInterpreter::transformOpaqueBuiltinTerm(OpaqueBuiltinTermPtr term) {
    errs() << "Unknown OpaqueBuiltinTerm: " << term->getName() << endl;
    return term;
}

Term::Ptr PSInterpreter::transformOpaqueCallTerm(OpaqueCallTermPtr term) {
    errs() << "Unknown OpaqueCallTerm: " << term->getName() << endl;
    return term;
}

Term::Ptr PSInterpreter::transformOpaqueFloatingConstantTerm(OpaqueFloatingConstantTermPtr term) {
    if (state_->find(term)) return term;
    state_->addConstant(term, DF_->getFloat(term->getValue()));
    return term;
}

Term::Ptr PSInterpreter::transformOpaqueIndexingTerm(OpaqueIndexingTermPtr term) {
    errs() << "Unknown OpaqueIndexingTerm: " << term->getName() << endl;
    return term;
}

Term::Ptr PSInterpreter::transformOpaqueIntConstantTerm(OpaqueIntConstantTermPtr term) {
    if (state_->find(term)) return term;
    Integer::Ptr integer;
    if (auto intTy = llvm::dyn_cast<type::Integer>(term->getType().get())) {
        auto apInt = llvm::APInt(intTy->getBitsize(), term->getValue(), intTy->getSignedness() == llvm::Signedness::Signed);
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

Term::Ptr PSInterpreter::transformOpaqueInvalidPtrTerm(OpaqueInvalidPtrTermPtr term) {
    if (state_->find(term)) return term;
    state_->addTerm(term, DF_->getNullptr(term->getType()));
    return term;
}

Term::Ptr PSInterpreter::transformOpaqueMemberAccessTerm(OpaqueMemberAccessTermPtr term) {
    errs() << "Unknown OpaqueMemberAccessTerm: " << term->getName() << endl;
    return term;
}

Term::Ptr PSInterpreter::transformOpaqueNamedConstantTerm(OpaqueNamedConstantTermPtr term) {
    errs() << "Unknown OpaqueNamedConstantTerm: " << term->getName() << endl;
    return term;
}

Term::Ptr PSInterpreter::transformOpaqueNullPtrTerm(OpaqueNullPtrTermPtr term) {
    if (state_->find(term)) return term;
    state_->addTerm(term, DF_->getNullptr(term->getType()));
    return term;
}

Term::Ptr PSInterpreter::transformOpaqueStringConstantTerm(OpaqueStringConstantTermPtr term) {
    if (state_->find(term)) return term;
    if (auto array = llvm::dyn_cast<type::Array>(term->getType())) {
        std::vector<Domain::Ptr> elements;
        for (auto&& it : term->getValue()) {
            auto ch2i = DF_->toInteger(it, 8);
            elements.push_back(DF_->getInteger(ch2i));
        }
        state_->addConstant(term, DF_->getAggregate(array->getElement(), elements));
    } else {
        state_->addTerm(term, DF_->getTop(term->getType()));
    }
    return term;
}

Term::Ptr PSInterpreter::transformOpaqueUndefTerm(OpaqueUndefTermPtr term) {
    state_->addTerm(term, DF_->getTop(term->getType()));
    return term;
}

Term::Ptr PSInterpreter::transformOpaqueVarTerm(OpaqueVarTermPtr term) {
    errs() << "Unknown OpaqueVarTerm: " << term->getName() << endl;
    return term;
}

Term::Ptr PSInterpreter::transformSignTerm(SignTermPtr term) {
    errs() << "Unknown SignTerm: " << term->getName() << endl;
    return term;
}

Term::Ptr PSInterpreter::transformTernaryTerm(TernaryTermPtr term) {
    if (state_->find(term)) return term;
    auto cond = state_->find(term->getCnd());
    auto trueVal = state_->find(term->getTru());
    auto falseVal = state_->find(term->getFls());
    ASSERT(cond && trueVal && falseVal, "ternary term args" + term->getName());

    Domain::Ptr result = cond->equals(DF_->getBool(true).get()) ? trueVal :
                         cond->equals(DF_->getBool(false).get()) ? falseVal :
                         DF_->getTop(term->getType());
    ASSERT(result, "ternary term result " + term->getName());
    state_->addTerm(term, result);
    return term;
}

Term::Ptr PSInterpreter::transformUnaryTerm(UnaryTermPtr term) {
    if (state_->find(term)) return term;
    auto rhv = state_->find(term->getRhv());
    ASSERT(rhv, "unary term arg " + term->getName());

    Domain::Ptr result;
    switch (term->getOpcode()) {
        case llvm::UnaryArithType::NEG: result = rhv->neg(); break;
        case llvm::UnaryArithType::NOT: result = rhv->neg(); break;
        case llvm::UnaryArithType::BNOT: result = rhv->neg(); break;
        default:
            UNREACHABLE("Unknown unary operator: " + term->getName());
    }
    state_->addTerm(term, result);
    return term;
}

Term::Ptr PSInterpreter::transformValueTerm(Transformer::ValueTermPtr term) {
    if (state_->find(term)) return term;
    if (term->isGlobal()) {
        auto domain = DF_->getGlobalVariableManager()->get(term->getVName());
        ASSERT(domain, "Unknown global variable: " + term->getName());
        state_->addTerm(term, domain);

    } else {
        state_->addTerm(term, DF_->getTop(term->getType()));
    }
    return term;
}

Term::Ptr PSInterpreter::transformVarArgumentTerm(VarArgumentTermPtr term) {
    errs() << "Unknown VarArgumentTerm: " << term->getName() << endl;
    return term;
}

}   // namespace absint
}   // namespace borealis

#include "Util/unmacros.h"