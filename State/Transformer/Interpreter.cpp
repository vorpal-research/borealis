//
// Created by abdullin on 10/20/17.
//

#include <Term/TermUtils.hpp>
#include "Interpreter.h"
#include "Interpreter/Domain/AbstractFactory.hpp"
#include "Interpreter/Domain/Memory/PointerDomain.hpp"
#include "Interpreter/Domain/Numerical/DoubleInterval.hpp"
#include "Interpreter/Domain/Numerical/Interval.hpp"
#include "Interpreter/Domain/VariableFactory.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {
namespace ps {

Interpreter::Interpreter(FactoryNest FN, const VariableFactory* vf)
        : Transformer(FN),
          ObjectLevelLogging("ps-interpreter"),
          vf_(vf),
          input_(std::make_shared<State>(vf_)),
          output_(std::make_shared<State>(vf_, input_)),
          equalities_(std::make_shared<TermMap>()) {}

Interpreter::Interpreter(FactoryNest FN, const VariableFactory* vf, State::Ptr input, TermMapPtr equalities)
        : Transformer(FN),
          ObjectLevelLogging("ps-interpreter"),
          vf_(vf),
          input_(input),
          output_(std::make_shared<State>(vf_, input_)),
          equalities_(equalities) {}

Interpreter::State::Ptr Interpreter::getState() const {
    return output_;
}

const Interpreter::TermMap& Interpreter::getEqualities() const {
    return *equalities_.get();
}

AbstractDomain::Ptr Interpreter::getDomain(Term::Ptr term) const {
    auto temp = input_->get(term);
    return temp ? temp : output_->get(term);
}

bool Interpreter::isConditionSatisfied(Predicate::Ptr pred, State::Ptr state) {
    if (auto&& eq = llvm::dyn_cast<EqualityPredicate>(pred.get())) {
        auto condition = state->get(eq->getLhv());
        ASSERT(condition, "Unknown condition in PATH predicate: " + pred->toString());
        if (condition->isTop() || condition->isBottom()) return true;

        // true cond
        if (eq->getRhv()->equals(FN.Term->getTrueTerm().get())) {
            auto boolean = llvm::dyn_cast<AbstractFactory::BoolT>(condition.get());
            ASSERTC(boolean);
            return boolean->isConstant(1);
        // false cond
        } else if (eq->getRhv()->equals(FN.Term->getFalseTerm().get())) {
            auto boolean = llvm::dyn_cast<AbstractFactory::BoolT>(condition.get());
            ASSERTC(boolean);
            return boolean->isConstant(0);
        // switch case
        } else if (auto intConst = llvm::dyn_cast<OpaqueIntConstantTerm>(eq->getRhv().get())) {
            auto&& intType = llvm::cast<type::Integer>(intConst->getType().get());
            auto integer = llvm::dyn_cast<AbstractFactory::IntT>(condition.get());
            ASSERT(integer, "Non-integer in choice condition");
            return integer->intersects(vf_->af()->getInteger(intConst->getValue(), intType->getBitsize()));
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

PredicateState::Ptr Interpreter::transformChoice(PredicateStateChoicePtr choice) {
    for (auto&& ch : choice->getChoices()) {
        interpretState(ch);
    }
    return choice;
}

PredicateState::Ptr Interpreter::transformChain(PredicateStateChainPtr chain) {
    auto baseInterpreter = Interpreter(FN, vf_, input_, equalities_);
    baseInterpreter.transform(chain->getBase());
    input_->joinWith(baseInterpreter.getState());

    auto currInterpreter = Interpreter(FN, vf_, input_, equalities_);
    currInterpreter.transform(chain->getCurr());
    if (output_->empty()) output_ = currInterpreter.getState();
    else output_->joinWith(currInterpreter.getState());
    return chain;
}

void Interpreter::interpretState(PredicateState::Ptr ps) {
    auto interpreter = Interpreter(FN, vf_, input_, equalities_);
    interpreter.transform(ps);
    if (output_->empty()) output_ = interpreter.getState();
    else output_->joinWith(interpreter.getState());
}

Predicate::Ptr Interpreter::transformAllocaPredicate(AllocaPredicatePtr pred) {
    output_->allocate(pred->getLhv(), pred->getOrigNumElems());
    return pred;
}

Predicate::Ptr Interpreter::transformCallPredicate(CallPredicatePtr pred) {
    if (pred->hasLhv()) {
        output_->assign(pred->getLhv(), vf_->top(pred->getLhv()->getType()));
    }
    return pred;
}

Predicate::Ptr Interpreter::transformDefaultSwitchCasePredicate(DefaultSwitchCasePredicatePtr pred) {
    auto condType = pred->getCond()->getType();
    auto result = vf_->bottom(condType);
    for (auto&& cs : pred->getCases()) result = result->join(getDomain(cs));
    // pred->getCond() != all cases, so add to state true branch of split TOP by neq with result
    // which is the same ass false branch of split TOP by eq with result
    output_->assign(pred->getCond(), vf_->top(condType)->splitByEq(result).false_);
    return pred;
}

Predicate::Ptr Interpreter::transformEqualityPredicate(EqualityPredicatePtr pred) {
    auto rhv = getDomain(pred->getRhv());
    ASSERT(rhv, "Equality rhv: " + pred->toString());
    output_->assign(pred->getLhv(), rhv);

    auto trueTerm = FN.Term->getTrueTerm();
    auto falseTerm = FN.Term->getFalseTerm();

    auto opt = util::at(*equalities_.get(), pred->getLhv());
    auto actualLhv = opt ? opt.getUnsafe() : pred->getLhv();
    // TODO: WTF is that?
    if (pred->getType() == PredicateType::PATH) {
        auto&& actDomain = getDomain(actualLhv);
        if (actDomain->isTop() or actDomain->isBottom()) {
            if (pred->getRhv()->equals(trueTerm.get())) {
                output_->assumeTrue(actualLhv);
            } else if (pred->getRhv()->equals(falseTerm.get())) {
                output_->assumeFalse(actualLhv);
            } else {
                auto&& cmp = FN.Term->getCmpTerm(llvm::ConditionType::EQ, pred->getLhv(), pred->getRhv());
                output_->assumeTrue(cmp);
            }
        }
    } else if (pred->getType() == PredicateType::ASSUME || pred->getType() == PredicateType::REQUIRES) {
        if (actualLhv->equals(trueTerm.get()) || actualLhv->equals(falseTerm.get())) {
            // do nothing if we have smth like (true = true)
        } else if (pred->getRhv()->equals(trueTerm.get())) {
            output_->assumeTrue(actualLhv);
        } else if (pred->getRhv()->equals(falseTerm.get())) {
            output_->assumeFalse(actualLhv);
        } else {
            warns() << "Unknown assume predicate: " << pred->toString() << endl;
        }
    } else {
        equalities_->operator[](pred->getLhv()) = pred->getRhv();
    }
    return pred;
}

Predicate::Ptr Interpreter::transformGlobalsPredicate(GlobalsPredicatePtr pred) {
    for (auto&& it : pred->getGlobals()) {
        output_->assign(it, vf_->top(it->getType()));
    }
    return pred;
}

Predicate::Ptr Interpreter::transformMallocPredicate(MallocPredicatePtr pred) {
    output_->allocate(pred->getLhv(), pred->getOrigNumElems());
    return pred;
}

Predicate::Ptr Interpreter::transformMarkPredicate(MarkPredicatePtr pred) {
    errs() << "Mark pred: " << pred->toString() << endl;
    output_->assign(pred->getId(), vf_->top(pred->getId()->getType()));
    return pred;
}

Predicate::Ptr Interpreter::transformStorePredicate(StorePredicatePtr pred) {
    output_->store(pred->getLhv(), pred->getRhv());
    return pred;
}

Term::Ptr Interpreter::transformArgumentCountTerm(ArgumentCountTermPtr term) {
    errs() << "Unknown ArgumentCountTerm: " << term->getName() << endl;
    return term;
}

Term::Ptr Interpreter::transformArgumentTerm(ArgumentTermPtr term) {
    output_->assign(term, vf_->top(term->getType()));
    return term;
}

Term::Ptr Interpreter::transformAxiomTerm(AxiomTermPtr term) {
    errs() << "Unknown AxiomTerm: " << term->getName() << endl;
    return term;
}

Term::Ptr Interpreter::transformBinaryTerm(BinaryTermPtr term) {
    output_->apply(term->getOpcode(), term, term->getLhv(), term->getRhv());
    return term;
}

Term::Ptr Interpreter::transformBoundTerm(BoundTermPtr term) {
    auto ptr = getDomain(term->getRhv());
    ASSERT(ptr, "bound term arg: " + term->getName());
    auto ptrDom = llvm::dyn_cast<AbstractFactory::PointerT>(ptr.get());
    ASSERT(ptrDom, "bound term arg is not a pointer" + ptr->toString());
    output_->assign(term, ptrDom->bound());
    return term;
}

Term::Ptr Interpreter::transformCastTerm(CastTermPtr term) {
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

    output_->apply(op, term, term->getRhv());
    return term;
}

Term::Ptr Interpreter::transformCmpTerm(CmpTermPtr term) {
    output_->apply(term->getOpcode(), term, term->getLhv(), term->getRhv());
    return term;
}

Term::Ptr Interpreter::transformConstTerm(ConstTermPtr term) {
    errs() << "Unknown ConstTerm: " << term->getName() << endl;
    return term;
}

Term::Ptr Interpreter::transformFreeVarTerm(FreeVarTermPtr term) {
    output_->assign(term, vf_->top(term->getType()));
    return term;
}

Term::Ptr Interpreter::transformGepTerm(GepTermPtr term) {
    output_->gep(term, term->getBase(), term->getShifts().toVector());
    return term;
}

Term::Ptr Interpreter::transformLoadTerm(LoadTermPtr term) {
    output_->load(term, term->getRhv());
    return term;
}

Term::Ptr Interpreter::transformOpaqueBigIntConstantTerm(OpaqueBigIntConstantTermPtr term) {
    return term;
}

Term::Ptr Interpreter::transformOpaqueBoolConstantTerm(OpaqueBoolConstantTermPtr term) {
    return term;
}

Term::Ptr Interpreter::transformOpaqueBuiltinTerm(OpaqueBuiltinTermPtr term) {
    return term;
}

Term::Ptr Interpreter::transformOpaqueCallTerm(OpaqueCallTermPtr term) {
    return term;
}

Term::Ptr Interpreter::transformOpaqueFloatingConstantTerm(OpaqueFloatingConstantTermPtr term) {
    return term;
}

Term::Ptr Interpreter::transformOpaqueIndexingTerm(OpaqueIndexingTermPtr term) {
    return term;
}

Term::Ptr Interpreter::transformOpaqueIntConstantTerm(OpaqueIntConstantTermPtr term) {
    return term;
}

Term::Ptr Interpreter::transformOpaqueInvalidPtrTerm(OpaqueInvalidPtrTermPtr term) {
    return term;
}

Term::Ptr Interpreter::transformOpaqueMemberAccessTerm(OpaqueMemberAccessTermPtr term) {
    return term;
}

Term::Ptr Interpreter::transformOpaqueNamedConstantTerm(OpaqueNamedConstantTermPtr term) {
    return term;
}

Term::Ptr Interpreter::transformOpaqueNullPtrTerm(OpaqueNullPtrTermPtr term) {
    return term;
}

Term::Ptr Interpreter::transformOpaqueStringConstantTerm(OpaqueStringConstantTermPtr term) {
    return term;
}

Term::Ptr Interpreter::transformOpaqueUndefTerm(OpaqueUndefTermPtr term) {
    return term;
}

Term::Ptr Interpreter::transformOpaqueVarTerm(OpaqueVarTermPtr term) {
    return term;
}

Term::Ptr Interpreter::transformSignTerm(SignTermPtr term) {
    return term;
}

Term::Ptr Interpreter::transformTernaryTerm(TernaryTermPtr term) {
    auto cond = getDomain(term->getCnd());
    auto trueVal = getDomain(term->getTru());
    auto falseVal = getDomain(term->getFls());
    ASSERT(cond && trueVal && falseVal, "ternary term args" + term->getName());

    auto&& result = cond->equals(vf_->af()->getBool(true)) ? trueVal :
            cond->equals(vf_->af()->getBool(false)) ? falseVal :
            trueVal->join(falseVal);
    ASSERT(result, "ternary term result " + term->getName());
    output_->assign(term, result);
    return term;
}

Term::Ptr Interpreter::transformUnaryTerm(UnaryTermPtr term) {
    // TODO: implement neg for numerical domains
    output_->assign(term, vf_->top(term->getType()));
    return term;
}

Term::Ptr Interpreter::transformValueTerm(Transformer::ValueTermPtr term) {
    if (getDomain(term)) return term;
    if (term->isGlobal()) {
        auto domain = vf_->findGlobal(term->getVName());
        ASSERT(domain, "Unknown global variable: " + term->getName());
        output_->assign(term, domain);

    } else {
        output_->assign(term, vf_->top(term->getType()));
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