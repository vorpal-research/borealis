/*
 * DefaultPredicateAnalysis.cpp
 *
 *  Created on: Aug 31, 2012
 *      Author: ice-phoenix
 */

#include <llvm/IR/InstVisitor.h>

#include <vector>
#include <Term/TermBuilder.h>

#include "Codegen/llvm.h"
#include "Codegen/intrinsics_manager.h"
#include "Passes/PredicateAnalysis/DefaultPredicateAnalysis.h"
#include "Passes/Tracker/SlotTrackerPass.h"

#include "Util/macros.h"

namespace borealis {

using borealis::util::tail;
using borealis::util::view;

////////////////////////////////////////////////////////////////////////////////
//
// Default predicate analysis instruction visitor
//
////////////////////////////////////////////////////////////////////////////////

static Term::Ptr isValidPtr(FactoryNest FN, Term::Ptr t) {
    auto type = t->getType();
    if (auto* ptrType = llvm::dyn_cast<type::Pointer>(type)) {
        auto&& pointed = ptrType->getPointed();

        auto&& bval = TermBuilder{ FN.Term, t };
        auto&& bound = bval.bound();

        return bval > FN.Term->getNullPtrTerm()->setType(FN.Term.get(), type)
               && bound.uge( FN.Term->getOpaqueConstantTerm(TypeUtils::getTypeSizeInElems(pointed), 0)->setType(FN.Term.get(), bound->getType()) );
    }
    return nullptr;
}

class DPAInstVisitor : public llvm::InstVisitor<DPAInstVisitor> {

public:

    DPAInstVisitor(DefaultPredicateAnalysis* pass) : pass(pass) {}

    void visitLoadInst(llvm::LoadInst& I) {
        using llvm::Value;

        Value* lhv = &I;
        Value* rhv = I.getPointerOperand();

        pass->PM[&I] = pass->FN.Predicate->getEqualityPredicate(
            pass->FN.Term->getValueTerm(lhv),
            pass->FN.Term->getLoadTerm(
                pass->FN.Term->getValueTerm(rhv)
            ),
            pass->SLT->getLocFor(&I),
            PredicateType::INVARIANT
        );

        pass->PostPM[&I] = pass->FN.Predicate->getEqualityPredicate(
            isValidPtr(pass->FN, pass->FN.Term->getValueTerm(rhv)),
            pass->FN.Term->getTrueTerm(),
            pass->SLT->getLocFor(&I),
            PredicateType::ASSUME
        );
    }

    void visitStoreInst(llvm::StoreInst& I) {
        using llvm::Value;

        Value* lhv = I.getPointerOperand();
        Value* rhv = I.getValueOperand();

        pass->PM[&I] = pass->FN.Predicate->getStorePredicate(
            pass->FN.Term->getValueTerm(lhv),
            pass->FN.Term->getValueTerm(rhv),
            pass->SLT->getLocFor(&I),
            PredicateType::INVARIANT
        );

        pass->PostPM[&I] = pass->FN.Predicate->getEqualityPredicate(
            isValidPtr(pass->FN, pass->FN.Term->getValueTerm(lhv)),
            pass->FN.Term->getTrueTerm(),
            pass->SLT->getLocFor(&I),
            PredicateType::ASSUME
        );
    }

    void visitCallInst(llvm::CallInst& I) {
        using llvm::Value;

        Term::Ptr lhv = I.getType()->isVoidTy() ?
                        nullptr :
                        pass->FN.Term->getValueTerm(&I);
        auto&& args = util::viewContainer(I.arg_operands()).map(APPLY(pass->FN.Term->getValueTerm)).toVector();
        auto&& loc = pass->SLT->getLocFor(&I);

        auto&& F = I.getCalledFunction();
        if (F != nullptr) {
            if (IntrinsicsManager::getInstance().getIntrinsicType(F) != function_type::UNKNOWN) return;

            Term::Ptr function = pass->FN.Term->getValueTerm(F);
            pass->PM[&I] = pass->FN.Predicate->getCallPredicate(lhv, function, args, loc);

        } else if (I.getCalledValue()) {
            pass->PM[&I] = pass->FN.Predicate->getCallPredicate(
                    lhv, pass->FN.Term->getValueTerm(I.getCalledValue()), args, loc
            );

        } else return;
    }


    void visitCmpInst(llvm::CmpInst& I) {
        using llvm::ConditionType;
        using llvm::Value;

        Value* lhv = &I;
        Value* op1 = I.getOperand(0);
        Value* op2 = I.getOperand(1);
        ConditionType cond = conditionType(I.getPredicate());

        pass->PM[&I] = pass->FN.Predicate->getEqualityPredicate(
            pass->FN.Term->getValueTerm(lhv),
            pass->FN.Term->getCmpTerm(
                cond,
                pass->FN.Term->getValueTerm(op1),
                pass->FN.Term->getValueTerm(op2)
            ),
            pass->SLT->getLocFor(&I),
            PredicateType::INVARIANT
        );
    }

    void visitBranchInst(llvm::BranchInst& I) {
        using llvm::BasicBlock;

        if (I.isUnconditional()) return;

        Term::Ptr condTerm = pass->FN.Term->getValueTerm(I.getCondition());
        const BasicBlock* trueSucc = I.getSuccessor(0);
        const BasicBlock* falseSucc = I.getSuccessor(1);

        pass->TPM[{&I, trueSucc}] =
            pass->FN.Predicate->getBooleanPredicate(
                condTerm,
                pass->FN.Term->getTrueTerm(),
                pass->SLT->getLocFor(&I)
            );
        pass->TPM[{&I, falseSucc}] =
            pass->FN.Predicate->getBooleanPredicate(
                condTerm,
                pass->FN.Term->getFalseTerm(),
                pass->SLT->getLocFor(&I)
            );
    }

    void visitSwitchInst(llvm::SwitchInst& I) {
        using llvm::BasicBlock;

        Term::Ptr condTerm = pass->FN.Term->getValueTerm(I.getCondition());

        std::vector<Term::Ptr> cases;
        cases.reserve(I.getNumCases());

        for (auto c = I.case_begin(); c != I.case_end(); ++c) {
            Term::Ptr caseTerm = pass->FN.Term->getConstTerm(c.getCaseValue());
            const BasicBlock* caseSucc = c.getCaseSuccessor();

            pass->TPM[{&I, caseSucc}] =
                pass->FN.Predicate->getEqualityPredicate(
                    condTerm,
                    caseTerm,
                    pass->SLT->getLocFor(&I),
                    PredicateType::PATH
                );

            cases.push_back(caseTerm);
        }

        const BasicBlock* defaultSucc = I.getDefaultDest();
        pass->TPM[{&I, defaultSucc}] =
            pass->FN.Predicate->getDefaultSwitchCasePredicate(
                condTerm,
                cases,
                pass->SLT->getLocFor(&I)
            );
    }

    void visitSelectInst(llvm::SelectInst& I) {
        using llvm::Value;

        Value* lhv = &I;
        Value* cnd = I.getCondition();
        Value* tru = I.getTrueValue();
        Value* fls = I.getFalseValue();

        pass->PM[&I] = pass->FN.Predicate->getEqualityPredicate(
            pass->FN.Term->getValueTerm(lhv),
            pass->FN.Term->getTernaryTerm(
                pass->FN.Term->getValueTerm(cnd),
                pass->FN.Term->getValueTerm(tru),
                pass->FN.Term->getValueTerm(fls)
            ),
            pass->SLT->getLocFor(&I),
            PredicateType::INVARIANT
        );
    }

    void visitGetElementPtrInst(llvm::GetElementPtrInst& I) {
        using namespace llvm;

        Value* lhv = &I;
        Value* rhv = I.getPointerOperand();

        std::vector<llvm::Value*> idxs;
        idxs.reserve(I.getNumOperands() - 1);
        for (auto& i : tail(view(I.op_begin(), I.op_end()))) {
            idxs.push_back(i);
        }

        pass->PM[&I] = pass->FN.Predicate->getEqualityPredicate(
            pass->FN.Term->getValueTerm(lhv),
            pass->FN.Term->getGepTerm(rhv, idxs, isTriviallyInboundsGEP(&I)),
            pass->SLT->getLocFor(&I),
            PredicateType::INVARIANT
        );

        pass->PostPM[&I] = pass->FN.Predicate->getEqualityPredicate(
            isValidPtr(pass->FN, pass->FN.Term->getValueTerm(rhv)),
            pass->FN.Term->getTrueTerm(),
            pass->SLT->getLocFor(&I),
            PredicateType::ASSUME
        );
    }

    void visitCastInst(llvm::CastInst& I) {
        using namespace llvm;

        Value* lhv = &I;
        Value* rhv = I.getOperand(0);
        bool signExtend = CastInst::CastOps::SExt == I.getOpcode();

        Term::Ptr lhvt = pass->FN.Term->getValueTerm(lhv);
        Term::Ptr rhvt = pass->FN.Term->getValueTerm(rhv);

        if (isa<type::Bool>(lhvt->getType()) and not isa<type::Bool>(rhvt->getType())) {
            rhvt = pass->FN.Term->getCmpTerm(
                ConditionType::NEQ,
                rhvt,
                pass->FN.Term->getIntTerm(0, rhvt->getType())
            );
        } else if (not isa<type::Bool>(lhvt->getType()) and isa<type::Bool>(rhvt->getType())) {
            lhvt = pass->FN.Term->getCmpTerm(
                ConditionType::NEQ,
                lhvt,
                pass->FN.Term->getIntTerm(0, lhvt->getType())
            );
        } else {
            rhvt = pass->FN.Term->getCastTerm(
                lhvt->getType(),
                signExtend,
                rhvt
            );
        }

        pass->PM[&I] = pass->FN.Predicate->getEqualityPredicate(
            lhvt,
            rhvt,
            pass->SLT->getLocFor(&I),
            PredicateType::INVARIANT
        );
    }

    void visitPHINode(llvm::PHINode& I) {
        using namespace llvm;

        for (unsigned int i = 0; i < I.getNumIncomingValues(); i++) {
            const BasicBlock* from = I.getIncomingBlock(i);
            Value* v = I.getIncomingValue(i);

            pass->PPM[{from, &I}] = pass->FN.Predicate->getEqualityPredicate(
                pass->FN.Term->getValueTerm(&I),
                pass->FN.Term->getValueTerm(v),
                pass->SLT->getLocFor(&I)
            );
        }
    }

    void visitBinaryOperator(llvm::BinaryOperator& I) {
        using namespace llvm;

        typedef llvm::Instruction::BinaryOps OPS;

        Value* lhv = &I;
        Value* op1 = I.getOperand(0);
        Value* op2 = I.getOperand(1);
        ArithType type = arithType(I.getOpcode());

        pass->PM[&I] = pass->FN.Predicate->getEqualityPredicate(
            pass->FN.Term->getValueTerm(lhv),
            pass->FN.Term->getBinaryTerm(
                type,
                pass->FN.Term->getValueTerm(op1),
                pass->FN.Term->getValueTerm(op2)
            ),
            pass->SLT->getLocFor(&I),
            PredicateType::INVARIANT
        );
    }

    void visitReturnInst(llvm::ReturnInst& I) {
        using llvm::Value;

        Value* rv = I.getReturnValue();
        if (rv == nullptr) return;

        pass->PM[&I] = pass->FN.Predicate->getEqualityPredicate(
            pass->FN.Term->getReturnValueTerm(I.getParent()->getParent()),
            pass->FN.Term->getValueTerm(rv),
            pass->SLT->getLocFor(&I)
        );
    }

private:

    DefaultPredicateAnalysis* pass;

};

////////////////////////////////////////////////////////////////////////////////

DefaultPredicateAnalysis::DefaultPredicateAnalysis() :
        ProxyFunctionPass(ID) {}

DefaultPredicateAnalysis::DefaultPredicateAnalysis(llvm::Pass* pass) :
        ProxyFunctionPass(ID, pass) {}

void DefaultPredicateAnalysis::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();

    AUX<SlotTrackerPass>::addRequiredTransitive(AU);
    AUX<SourceLocationTracker>::addRequiredTransitive(AU);
    AUX<llvm::DataLayoutPass>::addRequiredTransitive(AU);
}

bool DefaultPredicateAnalysis::runOnFunction(llvm::Function& F) {
    init();

    auto* st = GetAnalysis<SlotTrackerPass>::doit(this, F).getSlotTracker(F);
    FN = FactoryNest(F.getDataLayout(), st);

    SLT = &GetAnalysis<SourceLocationTracker>::doit(this, F);
    TD = &GetAnalysis<llvm::DataLayoutPass>::doit(this, F).getDataLayout();

    DPAInstVisitor visitor(this);
    visitor.visit(F);

    return false;
}

DefaultPredicateAnalysis::~DefaultPredicateAnalysis() {}

////////////////////////////////////////////////////////////////////////////////

char DefaultPredicateAnalysis::ID;
static RegisterPass<DefaultPredicateAnalysis>
X("default-predicate-analysis", "Default instruction predicate analysis");

} /* namespace borealis */

#include "Util/unmacros.h"
