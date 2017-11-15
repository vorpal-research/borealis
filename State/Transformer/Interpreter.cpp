//
// Created by abdullin on 10/20/17.
//

#include "Interpreter.h"
#include "Interpreter/Domain/DomainFactory.h"
#include "Interpreter/PredicateState/ConditionSplitter.h"

#include "Util/macros.h"
#include "ConditionExtractor.h"

namespace borealis {
namespace absint {
namespace ps {

Interpreter::Interpreter(FactoryNest FN, DomainFactory* DF, State::Ptr state, const TermMap& equalities)
        : Transformer(FN),
          ObjectLevelLogging("ps-interpreter"),
          DF_(DF),
          state_(std::make_shared<State>(state->getVariables(), state->getConstants())),
          equalities_(equalities) {}

State::Ptr Interpreter::getState() const {
    return state_;
}

const Interpreter::TermMap& Interpreter::getEqualities() const {
    return equalities_;
}

const Interpreter::StateMap& Interpreter::getStateMap() const {
    return states_;
}

bool Interpreter::isConditionSatisfied(Predicate::Ptr pred, State::Ptr state) {
    if (auto&& eq = llvm::dyn_cast<EqualityPredicate>(pred.get())) {
        auto condition = state->find(eq->getLhv());
        ASSERT(condition, "Unknown condition in PATH predicate: " + pred->toString());
        if (not condition->isValue()) return true;

        // true cond
        if (eq->getRhv()->equals(FN.Term->getTrueTerm().get())) {
            auto boolean = llvm::dyn_cast<IntegerIntervalDomain>(condition.get());
            ASSERT(boolean && boolean->getWidth() == 1, "Non-bool in choice condition");
            return boolean->isConstant(1);
        // false cond
        } else if (eq->getRhv()->equals(FN.Term->getFalseTerm().get())) {
            auto boolean = llvm::dyn_cast<IntegerIntervalDomain>(condition.get());
            ASSERT(boolean && boolean->getWidth() == 1, "Non-bool in choice condition");
            return boolean->isConstant(0);
        // switch case
        } else if (auto intConst = llvm::dyn_cast<OpaqueIntConstantTerm>(eq->getRhv().get())) {
            auto integer = llvm::dyn_cast<IntegerIntervalDomain>(condition.get());
            ASSERT(integer, "Non-integer in choice condition");
            return integer->hasIntersection(DF_->toInteger(intConst->getValue(), integer->getWidth()));
        } else {
            warns() << "Unexpected rhv in path predicate: " << pred->toString() << endl;
        }
    } else if (llvm::isa<DefaultSwitchCasePredicate>(pred.get())) {
        return true;
    } else {
        UNREACHABLE("Unknown path predicate: " + pred->toString())
    }
    return true;
}

PredicateState::Ptr Interpreter::transformBasic(BasicPredicateStatePtr basic) {
    auto result = Base::transformBasic(basic);
    states_[basic] = state_;
    return result;
}

PredicateState::Ptr Interpreter::transformChoice(PredicateStateChoicePtr choice) {
    for (auto&& ch : choice->getChoices()) {
        ConditionExtractor exteactor(FN);
        exteactor.transform(ch);
        ASSERT(exteactor.getConditions().size() > 0, "Empty vector of conditions");

        // we need this to add terms from this predicate to state
        auto interpreter = Interpreter(FN, DF_, state_, equalities_);
        interpreter.transform(FN.State->Basic({exteactor.getConditions()[0]}));

        if (isConditionSatisfied(exteactor.getConditions()[0], interpreter.getState())) {
            interpretState(ch);
        }
    }
    return choice;
}

PredicateState::Ptr Interpreter::transformChain(PredicateStateChainPtr chain) {
    interpretState(chain->getBase());
    interpretState(chain->getCurr());
    return chain;
}

void Interpreter::interpretState(PredicateState::Ptr ps) {
    auto interpreter = Interpreter(FN, DF_, state_, equalities_);
    interpreter.transform(ps);
    for (auto&& it : interpreter.getEqualities()) {
        equalities_[it.first] = it.second;
    }
    state_->merge(interpreter.getState());
    for (auto&& it : interpreter.getStateMap()) {
        ASSERT(states_.find(it.first) == states_.end(), "found new states map");
        states_.insert(it);
    }
}

Predicate::Ptr Interpreter::transformAllocaPredicate(AllocaPredicatePtr pred) {
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
    state_->addVariable(pred->getLhv(), domain);
    return pred;
}

Predicate::Ptr Interpreter::transformCallPredicate(Transformer::CallPredicatePtr pred) {
    if (pred->hasLhv()) {
        state_->addVariable(pred->getLhv(), DF_->getTop(pred->getLhv()->getType()));
    }
    return pred;
}

Predicate::Ptr Interpreter::transformDefaultSwitchCasePredicate(DefaultSwitchCasePredicatePtr pred) {
    auto condType = pred->getCond()->getType();
    auto result = DF_->getBottom(condType);
    for (auto&& cs : pred->getCases()) result = result->join(state_->find(cs));
    // pred->getCond() != all cases, so add to state true branch of split TOP by neq with result
    // which is the same ass false branch of split TOP by eq with result
    state_->addVariable(pred->getCond(), DF_->getTop(condType)->splitByEq(result).false_);
    return pred;
}

Predicate::Ptr Interpreter::transformEqualityPredicate(EqualityPredicatePtr pred) {
    auto rhv = state_->find(pred->getRhv());
    ASSERT(rhv, "Equality rhv: " + pred->toString());
    state_->addVariable(pred->getLhv(), rhv);

    auto trueTerm = FN.Term->getTrueTerm();
    auto falseTerm = FN.Term->getFalseTerm();
    if (pred->getType() == PredicateType::PATH) {
        if (not rhv->isValue()) {
            if (pred->getRhv()->equals(trueTerm.get())) {
                for (auto&& it : ConditionSplitter(equalities_[pred->getLhv()], state_).apply()) {
                    state_->addVariable(it.first, it.second.true_);
                }
            } else if (pred->getRhv()->equals(falseTerm.get())) {
                for (auto&& it : ConditionSplitter(equalities_[pred->getLhv()], state_).apply()) {
                    state_->addVariable(it.first, it.second.false_);
                }
            } else {
                warns() << "Unknown path predicate: " << pred->toString() << endl;
            }
        }
    } else if (pred->getType() == PredicateType::ASSUME || pred->getType() == PredicateType::REQUIRES) {
        auto&& lhv = equalities_[pred->getLhv()];
        auto&& actualLhv = lhv ? lhv : pred->getLhv();
        if (actualLhv->equals(trueTerm.get()) || actualLhv->equals(falseTerm.get())) {
            // do nothing if we have smth like (true = true)
        } else if (pred->getRhv()->equals(trueTerm.get())) {
            for (auto&& it : ConditionSplitter(actualLhv, state_).apply()) {
                state_->addVariable(it.first, it.second.true_);
            }
        } else if (pred->getRhv()->equals(falseTerm.get())) {
            for (auto&& it : ConditionSplitter(actualLhv, state_).apply()) {
                state_->addVariable(it.first, it.second.false_);
            }
        } else {
            warns() << "Unknown assume predicate: " << pred->toString() << endl;
        }
    } else {
        equalities_[pred->getLhv()] = pred->getRhv();
    }
    return pred;
}

Predicate::Ptr Interpreter::transformGlobalsPredicate(GlobalsPredicatePtr pred) {
    for (auto&& it : pred->getGlobals()) {
        state_->addVariable(it, DF_->getTop(it->getType()));
    }
    return pred;
}

Predicate::Ptr Interpreter::transformInequalityPredicate(InequalityPredicatePtr pred) {
    errs() << "Inequality pred: " << pred->toString() << endl;
    state_->addVariable(pred->getLhv(), DF_->getTop(pred->getLhv()->getType()));
    return pred;
}

Predicate::Ptr Interpreter::transformMallocPredicate(MallocPredicatePtr pred) {
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
    state_->addVariable(pred->getLhv(), domain);
    return pred;
}

Predicate::Ptr Interpreter::transformMarkPredicate(MarkPredicatePtr pred) {
    errs() << "Mark pred: " << pred->toString() << endl;
    state_->addVariable(pred->getId(), DF_->getTop(pred->getId()->getType()));
    return pred;
}

Predicate::Ptr Interpreter::transformStorePredicate(StorePredicatePtr pred) {
    auto ptr = state_->find(pred->getLhv());
    auto storeVal = state_->find(pred->getRhv());
    ASSERT(ptr && storeVal, "store args of " + pred->toString());

    ptr->store(storeVal, DF_->getIndex(0));
    return pred;
}

Term::Ptr Interpreter::transformArgumentCountTerm(ArgumentCountTermPtr term) {
    errs() << "Unknown ArgumentCountTerm: " << term->getName() << endl;
    return term;
}

Term::Ptr Interpreter::transformArgumentTerm(ArgumentTermPtr term) {
    if (state_->find(term)) return term;
    state_->addVariable(term, DF_->getTop(term->getType()));
    return term;
}

Term::Ptr Interpreter::transformAxiomTerm(AxiomTermPtr term) {
    errs() << "Unknown AxiomTerm: " << term->getName() << endl;
    return term;
}

Term::Ptr Interpreter::transformBinaryTerm(BinaryTermPtr term) {
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
    state_->addVariable(term, result);
    return term;
}

Term::Ptr Interpreter::transformBoundTerm(BoundTermPtr term) {
    auto ptr = state_->find(term->getRhv());
    ASSERT(ptr, "bound term arg: " + term->getName());
    auto ptrDom = llvm::dyn_cast<PointerDomain>(ptr.get());
    ASSERT(ptrDom, "bound term arg is not a pointer" + ptr->toString());
    state_->addVariable(term, ptrDom->getBound());
    return term;
}

Term::Ptr Interpreter::transformCastTerm(CastTermPtr term) {
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
    state_->addVariable(term, result);
    return term;
}

Term::Ptr Interpreter::transformCmpTerm(CmpTermPtr term) {
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

Term::Ptr Interpreter::transformConstTerm(ConstTermPtr term) {
    errs() << "Unknown ConstTerm: " << term->getName() << endl;
    return term;
}

Term::Ptr Interpreter::transformFreeVarTerm(FreeVarTermPtr term) {
    state_->addVariable(term, DF_->getTop(term->getType()));
    return term;
}

Term::Ptr Interpreter::transformGepTerm(GepTermPtr term) {
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
    state_->addVariable(term, result);
    return term;
}

Term::Ptr Interpreter::transformLoadTerm(LoadTermPtr term) {
    if (state_->find(term)) return term;
    auto ptr = state_->find(term->getRhv());
    ASSERT(ptr, "load pointer of " + term->getName());

    auto result = ptr->load(term->getType(), DF_->getIndex(0));
    ASSERT(result, "load result " + term->getName());
    state_->addVariable(term, result);
    return term;
}

Term::Ptr Interpreter::transformOpaqueBigIntConstantTerm(OpaqueBigIntConstantTermPtr term) {
    if (state_->find(term)) return term;
    auto intTy = llvm::cast<type::Integer>(term->getType().get());
    auto integer = DF_->toInteger(llvm::APInt(intTy->getBitsize(), term->getRepresentation(), 10));
    state_->addConstant(term, DF_->getInteger(integer));
    return term;
}

Term::Ptr Interpreter::transformOpaqueBoolConstantTerm(OpaqueBoolConstantTermPtr term) {
    if (state_->find(term)) return term;
    state_->addConstant(term, DF_->getBool(term->getValue()));
    return term;
}

Term::Ptr Interpreter::transformOpaqueBuiltinTerm(OpaqueBuiltinTermPtr term) {
    errs() << "Unknown OpaqueBuiltinTerm: " << term->getName() << endl;
    return term;
}

Term::Ptr Interpreter::transformOpaqueCallTerm(OpaqueCallTermPtr term) {
    errs() << "Unknown OpaqueCallTerm: " << term->getName() << endl;
    return term;
}

Term::Ptr Interpreter::transformOpaqueFloatingConstantTerm(OpaqueFloatingConstantTermPtr term) {
    if (state_->find(term)) return term;
    state_->addConstant(term, DF_->getFloat(term->getValue()));
    return term;
}

Term::Ptr Interpreter::transformOpaqueIndexingTerm(OpaqueIndexingTermPtr term) {
    errs() << "Unknown OpaqueIndexingTerm: " << term->getName() << endl;
    return term;
}

Term::Ptr Interpreter::transformOpaqueIntConstantTerm(OpaqueIntConstantTermPtr term) {
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

Term::Ptr Interpreter::transformOpaqueInvalidPtrTerm(OpaqueInvalidPtrTermPtr term) {
    if (state_->find(term)) return term;
    state_->addVariable(term, DF_->getNullptr(term->getType()));
    return term;
}

Term::Ptr Interpreter::transformOpaqueMemberAccessTerm(OpaqueMemberAccessTermPtr term) {
    errs() << "Unknown OpaqueMemberAccessTerm: " << term->getName() << endl;
    return term;
}

Term::Ptr Interpreter::transformOpaqueNamedConstantTerm(OpaqueNamedConstantTermPtr term) {
    errs() << "Unknown OpaqueNamedConstantTerm: " << term->getName() << endl;
    return term;
}

Term::Ptr Interpreter::transformOpaqueNullPtrTerm(OpaqueNullPtrTermPtr term) {
    if (state_->find(term)) return term;
    state_->addVariable(term, DF_->getNullptr(term->getType()));
    return term;
}

Term::Ptr Interpreter::transformOpaqueStringConstantTerm(OpaqueStringConstantTermPtr term) {
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

Term::Ptr Interpreter::transformOpaqueUndefTerm(OpaqueUndefTermPtr term) {
    state_->addVariable(term, DF_->getTop(term->getType()));
    return term;
}

Term::Ptr Interpreter::transformOpaqueVarTerm(OpaqueVarTermPtr term) {
    errs() << "Unknown OpaqueVarTerm: " << term->getName() << endl;
    return term;
}

Term::Ptr Interpreter::transformSignTerm(SignTermPtr term) {
    errs() << "Unknown SignTerm: " << term->getName() << endl;
    return term;
}

Term::Ptr Interpreter::transformTernaryTerm(TernaryTermPtr term) {
    if (state_->find(term)) return term;
    auto cond = state_->find(term->getCnd());
    auto trueVal = state_->find(term->getTru());
    auto falseVal = state_->find(term->getFls());
    ASSERT(cond && trueVal && falseVal, "ternary term args" + term->getName());

    Domain::Ptr result = cond->equals(DF_->getBool(true).get()) ? trueVal :
                         cond->equals(DF_->getBool(false).get()) ? falseVal :
                         DF_->getTop(term->getType());
    ASSERT(result, "ternary term result " + term->getName());
    state_->addVariable(term, result);
    return term;
}

Term::Ptr Interpreter::transformUnaryTerm(UnaryTermPtr term) {
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
    state_->addVariable(term, result);
    return term;
}

Term::Ptr Interpreter::transformValueTerm(Transformer::ValueTermPtr term) {
    if (state_->find(term)) return term;
    if (term->isGlobal()) {
        auto domain = DF_->getGlobalVariableManager()->get(term->getVName());
        ASSERT(domain, "Unknown global variable: " + term->getName());
        state_->addVariable(term, domain);

    } else {
        state_->addVariable(term, DF_->getTop(term->getType()));
    }
    return term;
}

Term::Ptr Interpreter::transformVarArgumentTerm(VarArgumentTermPtr term) {
    errs() << "Unknown VarArgumentTerm: " << term->getName() << endl;
    return term;
}

}   // namespace ps
}   // namespace absint
}   // namespace borealis

#include "Util/unmacros.h"