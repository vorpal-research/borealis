////
//// Created by abdullin on 11/22/17.
////
//
//#include "Interpreter/Domain/DomainFactory.h"
//#include "Interpreter/Domain/IntegerIntervalDomain.h"
//#include "QueryChecker.h"
//
//#include "Util/macros.h"
//
//namespace borealis {
//namespace absint {
//namespace ps {
//
//QueryChecker::QueryChecker(FactoryNest FN, DomainFactory* DF, State::Ptr state)
//        : Transformer(FN),
//          ObjectLevelLogging("ps-interpreter"),
//          satisfied_(true),
//          DF_(DF),
//          state_(state) {}
//
//bool QueryChecker::satisfied() const {
//    return satisfied_;
//}
//
//Predicate::Ptr QueryChecker::transformEqualityPredicate(EqualityPredicatePtr pred) {
//    if (not satisfied()) return pred;
//
//    auto trueTerm = FN.Term->getTrueTerm();
//    auto falseTerm = FN.Term->getFalseTerm();
//    auto cond = state_->find(pred->getLhv());
//    ASSERT(cond, "Unknown lhv in query");
//
//    if (pred->getRhv()->equals(trueTerm.get())) {
//        auto boolean = llvm::dyn_cast<IntegerIntervalDomain>(cond.get());
//        ASSERT(boolean && boolean->getWidth() == 1, "Non-bool in branch condition");
//        satisfied_ &= boolean->isConstant(1);
//
//    } else if (pred->getRhv()->equals(falseTerm.get())) {
//        auto boolean = llvm::dyn_cast<IntegerIntervalDomain>(cond.get());
//        ASSERT(boolean && boolean->getWidth() == 1, "Non-bool in branch condition");
//        satisfied_ &= boolean->isConstant(0);
//
//    } else {
//        warns() << "Unknown query predicate: " << pred->toString() << endl;
//    }
//
//    return pred;
//}
//
//Predicate::Ptr QueryChecker::transformInequalityPredicate(InequalityPredicatePtr pred) {
//    if (not satisfied()) return pred;
//
//    auto trueTerm = FN.Term->getTrueTerm();
//    auto falseTerm = FN.Term->getFalseTerm();
//    auto lhv = state_->find(pred->getLhv());
//    auto rhv = state_->find(pred->getRhv());
//    ASSERT(lhv && rhv, "Unknown predicate in query:" + pred->toString());
//
//    auto&& isFloat = llvm::isa<FloatIntervalDomain>(lhv.get());
//    auto res = isFloat ?
//               lhv->fcmp(rhv, llvm::CmpInst::FCMP_UNE) :
//               lhv->icmp(rhv, llvm::CmpInst::ICMP_NE);
//    auto boolean = llvm::dyn_cast<IntegerIntervalDomain>(res.get());
//    ASSERT(boolean && boolean->getWidth() == 1, "Non-bool in branch condition");
//    satisfied_ &= boolean->isConstant(1);
//
//    return pred;
//}
//
//Term::Ptr QueryChecker::transformArgumentTerm(ArgumentTermPtr term) {
//    auto&& domain = state_->find(term);
//    state_->addVariable(term, domain ? domain : DF_->getTop(term->getType()));
//    return term;
//}
//
//Term::Ptr QueryChecker::transformBinaryTerm(BinaryTermPtr term) {
//    if (not satisfied()) return term;
//    if (state_->find(term)) return term;
//    auto lhv = state_->find(term->getLhv());
//    auto rhv = state_->find(term->getRhv());
//    ASSERT(lhv && rhv, "binop args of " + term->getName());
//    auto isFloat = llvm::isa<type::Float>(term->getType().get());
//
//    Domain::Ptr result = nullptr;
//    switch (term->getOpcode()) {
//        case llvm::ArithType::ADD:
//            result = isFloat ?
//                     lhv->fadd(rhv) :
//                     lhv->add(rhv);
//            break;
//        case llvm::ArithType::SUB:
//            result = isFloat ?
//                     lhv->fsub(rhv) :
//                     lhv->sub(rhv);
//            break;
//        case llvm::ArithType::MUL:
//            result = isFloat ?
//                     lhv->fmul(rhv) :
//                     lhv->mul(rhv);
//            break;
//        case llvm::ArithType::DIV:
//            if (isFloat) result = lhv->fdiv(rhv);
//            else if (auto intty = llvm::dyn_cast<type::Integer>(term->getType().get())) {
//                result = (intty->getSignedness() == llvm::Signedness::Signed) ?
//                         lhv->sdiv(rhv) :
//                         lhv->udiv(rhv);
//            }
//            break;
//        case llvm::ArithType::REM:
//            if (isFloat) result = lhv->frem(rhv);
//            else if (auto intty = llvm::dyn_cast<type::Integer>(term->getType().get())) {
//                result = (intty->getSignedness() == llvm::Signedness::Signed) ?
//                         lhv->srem(rhv) :
//                         lhv->urem(rhv);
//            }
//            break;
//        case llvm::ArithType::SHL:
//            result = lhv->shl(rhv);
//            break;
//        case llvm::ArithType::LSHR:
//            result = lhv->lshr(rhv);
//            break;
//        case llvm::ArithType::ASHR:
//            result = lhv->ashr(rhv);
//            break;
//        case llvm::ArithType::LAND:
//        case llvm::ArithType::BAND:
//            result = lhv->bAnd(rhv);
//            break;
//        case llvm::ArithType::LOR:
//        case llvm::ArithType::BOR:
//            result = lhv->bOr(rhv);
//            break;
//        case llvm::ArithType::XOR:
//            result = lhv->bXor(rhv);
//            break;
//        case llvm::ArithType::IMPLIES:
//            result = lhv->implies(rhv);
//            break;
//        default:
//            UNREACHABLE("Unknown binary operator in query: " + term->getName());
//    }
//
//    ASSERT(result, "binop result " + term->getName());
//    state_->addVariable(term, result);
//    return term;
//}
//
//Term::Ptr QueryChecker::transformBoundTerm(BoundTermPtr term) {
//    if (not satisfied()) return term;
//    auto ptr = state_->find(term->getRhv());
//    ASSERT(ptr, "bound term arg: " + term->getName());
//    auto ptrDom = llvm::dyn_cast<PointerDomain>(ptr.get());
//    ASSERT(ptrDom, "bound term arg is not a pointer" + ptr->toString());
//    state_->addVariable(term, ptrDom->getBound());
//    return term;
//}
//
//Term::Ptr QueryChecker::transformCastTerm(CastTermPtr term) {
//    if (not satisfied()) return term;
//    auto cast = state_->find(term->getRhv());
//    ASSERT(cast, "cast arg of " + term->getName());
//    auto fromTy = term->getRhv()->getType();
//    auto toTy = term->getType();
//
//    Domain::Ptr result;
//    if (auto m = util::match_tuple<type::Integer, type::Integer>::doit(fromTy, toTy)) {
//        auto fromBitsize = m->get<0>()->getBitsize();
//        auto toBitsize = m->get<1>()->getBitsize();
//        if (toBitsize < fromBitsize) result = cast->trunc(toTy);
//        else if (toBitsize == fromBitsize) result = cast;
//        else result = term->isSignExtend() ?
//                      cast->sext(toTy) :
//                      cast->zext(toTy);
//
//    } else if (auto match = util::match_tuple<type::Float, type::Float>::doit(fromTy, toTy)) {
//        result = cast;
//    } else if (auto match = util::match_tuple<type::Float, type::Integer>::doit(fromTy, toTy)) {
//        result = (match->get<1>()->getSignedness() == llvm::Signedness::Signed) ?
//                 cast->fptosi(toTy) :
//                 cast->fptoui(toTy);
//    } else if (auto match = util::match_tuple<type::Integer, type::Float>::doit(fromTy, toTy)) {
//        result = (match->get<0>()->getSignedness() == llvm::Signedness::Signed) ?
//                 cast->sitofp(toTy) :
//                 cast->uitofp(toTy);
//    } else if (auto match = util::match_tuple<type::Pointer, type::Integer>::doit(fromTy, toTy)) {
//        result = cast->ptrtoint(toTy);
//    } else if (auto match = util::match_tuple<type::Integer, type::Pointer>::doit(fromTy, toTy)) {
//        result = cast->inttoptr(toTy);
//    } else if (auto match = util::match_tuple<type::Pointer, type::Pointer>::doit(fromTy, toTy)) {
//        result = cast->bitcast(toTy);
//    } else {
//        UNREACHABLE("Unknown cast: " + term->getName());
//    }
//
//    ASSERT(result, "cast result " + term->getName());
//    state_->addVariable(term, result);
//    return term;
//}
//
//Term::Ptr QueryChecker::transformCmpTerm(CmpTermPtr term) {
//    if (not satisfied()) return term;
//    if (state_->find(term)) return term;
//    auto lhv = state_->find(term->getLhv());
//    auto rhv = state_->find(term->getRhv());
//    ASSERT(lhv && rhv, "cmp args of " + term->getName());
//
//    bool isFloat = llvm::isa<type::Float>(term->getLhv()->getType().get());
//    Domain::Ptr result;
//    switch (term->getOpcode()) {
//        case llvm::ConditionType::EQ:
//            result = isFloat ?
//                     lhv->fcmp(rhv, llvm::CmpInst::FCMP_UEQ) :
//                     lhv->icmp(rhv, llvm::CmpInst::ICMP_EQ);
//            break;
//        case llvm::ConditionType::NEQ:
//            result = isFloat ?
//                     lhv->fcmp(rhv, llvm::CmpInst::FCMP_UNE) :
//                     lhv->icmp(rhv, llvm::CmpInst::ICMP_NE);
//            break;
//        case llvm::ConditionType::GT:
//            result = isFloat ?
//                     lhv->fcmp(rhv, llvm::CmpInst::FCMP_UGT) :
//                     lhv->icmp(rhv, llvm::CmpInst::ICMP_SGT);
//            break;
//        case llvm::ConditionType::GE:
//            result = isFloat ?
//                     lhv->fcmp(rhv, llvm::CmpInst::FCMP_UGE) :
//                     lhv->icmp(rhv, llvm::CmpInst::ICMP_SGE);
//            break;
//        case llvm::ConditionType::LT:
//            result = isFloat ?
//                     lhv->fcmp(rhv, llvm::CmpInst::FCMP_ULT) :
//                     lhv->icmp(rhv, llvm::CmpInst::ICMP_SLT);
//            break;
//        case llvm::ConditionType::LE:
//            result = isFloat ?
//                     lhv->fcmp(rhv, llvm::CmpInst::FCMP_ULE) :
//                     lhv->icmp(rhv, llvm::CmpInst::ICMP_SLE);
//            break;
//        case llvm::ConditionType::UGT:
//            result = isFloat ?
//                     lhv->fcmp(rhv, llvm::CmpInst::FCMP_UGT) :
//                     lhv->icmp(rhv, llvm::CmpInst::ICMP_UGT);
//            break;
//        case llvm::ConditionType::UGE:
//            result = isFloat ?
//                     lhv->fcmp(rhv, llvm::CmpInst::FCMP_UGE) :
//                     lhv->icmp(rhv, llvm::CmpInst::ICMP_UGE);
//            break;
//        case llvm::ConditionType::ULT:
//            result = isFloat ?
//                     lhv->fcmp(rhv, llvm::CmpInst::FCMP_ULT) :
//                     lhv->icmp(rhv, llvm::CmpInst::ICMP_ULT);
//            break;
//        case llvm::ConditionType::ULE:
//            result = isFloat ?
//                     lhv->fcmp(rhv, llvm::CmpInst::FCMP_ULE) :
//                     lhv->icmp(rhv, llvm::CmpInst::ICMP_ULE);
//            break;
//        case llvm::ConditionType::TRUE:
//            result = DF_->getBool(true);
//            break;
//        case llvm::ConditionType::FALSE:
//            result = DF_->getBool(false);
//            break;
//        default:
//            UNREACHABLE("Unknown cast: " + term->getName());
//    }
//
//    ASSERT(result, "cmp result " + term->getName());
//    state_->addVariable(term, result);
//    return term;
//}
//
//Term::Ptr QueryChecker::transformOpaqueBigIntConstantTerm(OpaqueBigIntConstantTermPtr term) {
//    if (not satisfied()) return term;
//    if (state_->find(term)) return term;
//    auto intTy = llvm::cast<type::Integer>(term->getType().get());
//    auto integer = DF_->toInteger(llvm::APInt(intTy->getBitsize(), term->getRepresentation(), 10));
//    state_->addConstant(term, DF_->getInteger(integer));
//    return term;
//}
//
//Term::Ptr QueryChecker::transformOpaqueBoolConstantTerm(OpaqueBoolConstantTermPtr term) {
//    if (not satisfied()) return term;
//    if (state_->find(term)) return term;
//    state_->addConstant(term, DF_->getBool(term->getValue()));
//    return term;
//}
//
//Term::Ptr QueryChecker::transformOpaqueFloatingConstantTerm(OpaqueFloatingConstantTermPtr term) {
//    if (not satisfied()) return term;
//    if (state_->find(term)) return term;
//    state_->addConstant(term, DF_->getFloat(term->getValue()));
//    return term;
//}
//
//Term::Ptr QueryChecker::transformOpaqueIntConstantTerm(OpaqueIntConstantTermPtr term) {
//    if (not satisfied()) return term;
//    if (state_->find(term)) return term;
//    Integer::Ptr integer;
//    if (auto intTy = llvm::dyn_cast<type::Integer>(term->getType().get())) {
//        auto apInt = llvm::APInt(intTy->getBitsize(), term->getValue(),
//                                 intTy->getSignedness() == llvm::Signedness::Signed);
//        integer = DF_->toInteger(apInt);
//        state_->addConstant(term, DF_->getInteger(integer));
//
//    } else if (llvm::isa<type::Pointer>(term->getType().get())) {
//        state_->addConstant(term, (term->getValue() == 0) ?
//                                  DF_->getNullptr(term->getType()) :
//                                  DF_->getTop(term->getType()));
//    } else {
//        warns() << "Unknown type in OpaqueIntConstant: " << TypeUtils::toString(*term->getType().get()) << endl;
//        state_->addConstant(term, DF_->getIndex(term->getValue()));
//    };
//    return term;
//}
//
//Term::Ptr QueryChecker::transformOpaqueInvalidPtrTerm(OpaqueInvalidPtrTermPtr term) {
//    if (not satisfied()) return term;
//    if (state_->find(term)) return term;
//    state_->addVariable(term, DF_->getNullptr(term->getType()));
//    return term;
//}
//
//Term::Ptr QueryChecker::transformOpaqueNullPtrTerm(OpaqueNullPtrTermPtr term) {
//    if (not satisfied()) return term;
//    if (state_->find(term)) return term;
//    state_->addVariable(term, DF_->getNullptr(term->getType()));
//    return term;
//}
//
//Term::Ptr QueryChecker::transformOpaqueStringConstantTerm(OpaqueStringConstantTermPtr term) {
//    if (not satisfied()) return term;
//    if (state_->find(term)) return term;
//    if (auto array = llvm::dyn_cast<type::Array>(term->getType())) {
//        std::vector<Domain::Ptr> elements;
//        for (auto&& it : term->getValue()) {
//            auto ch2i = DF_->toInteger(it, 8);
//            elements.push_back(DF_->getInteger(ch2i));
//        }
//        state_->addConstant(term, DF_->getAggregate(array->getElement(), elements));
//    } else {
//        state_->addVariable(term, DF_->getTop(term->getType()));
//    }
//    return term;
//}
//
//Term::Ptr QueryChecker::transformGepTerm(GepTermPtr term) {
//    if (not satisfied()) return term;
//    auto ptr = state_->find(term->getBase());
//    ASSERT(ptr, "gep pointer of " + term->getName());
//
//    std::vector<Domain::Ptr> shifts;
//    for (auto&& it : term->getShifts()) {
//        auto shift = state_->find(it);
//
//        auto&& integer = llvm::dyn_cast<IntegerIntervalDomain>(shift.get());
//        ASSERT(integer, "Non-integer domain in gep shift");
//
//        if (integer->getWidth() < DF_->defaultSize) {
//            shifts.emplace_back(shift->zext(FN.Type->getInteger(DF_->defaultSize)));
//        } else if (integer->getWidth() > DF_->defaultSize) {
//            shifts.emplace_back(shift->trunc(FN.Type->getInteger(DF_->defaultSize)));
//        } else {
//            shifts.emplace_back(shift);
//        }
//    }
//    auto elementType = llvm::cast<type::Pointer>(term->getType().get())->getPointed();
//    auto result = ptr->gep(elementType, shifts);
//
//    ASSERT(result, "gep result " + term->getName());
//    state_->addVariable(term, result);
//    return term;
//}
//
//Term::Ptr QueryChecker::transformOpaqueUndefTerm(OpaqueUndefTermPtr term) {
//    if (not satisfied()) return term;
//    state_->addVariable(term, DF_->getTop(term->getType()));
//    return term;
//}
//
//Term::Ptr QueryChecker::transformReadPropertyTerm(Transformer::ReadPropertyTermPtr term) {
//    satisfied_ = false;
//    return term;
//}
//
//Term::Ptr QueryChecker::transformValueTerm(Transformer::ValueTermPtr term) {
//    if (state_->find(term)) return term;
//    if (term->isGlobal()) {
//        auto domain = DF_->getGlobalVariableManager()->get(term->getVName());
//        ASSERT(domain, "Unknown global variable: " + term->getName());
//        state_->addVariable(term, domain);
//
//    } else {
//        state_->addVariable(term, DF_->getTop(term->getType()));
//    }
//    return term;
//}
//
//} // namespace ps
//} // namespace absint
//} // namespace borealis
//
//#include "Util/unmacros.h"