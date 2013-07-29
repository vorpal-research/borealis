/*
 * DefaultPredicateAnalysis.cpp
 *
 *  Created on: Aug 31, 2012
 *      Author: ice-phoenix
 */

#include <llvm/Support/InstVisitor.h>

#include <vector>

#include "Codegen/llvm.h"
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

class DPAInstVisitor : public llvm::InstVisitor<DPAInstVisitor> {

public:

    DPAInstVisitor(DefaultPredicateAnalysis* pass) : pass(pass) {}

    void visitLoadInst(llvm::LoadInst& I) {
        using llvm::Value;

        Value* lhv = &I;
        Value* rhv = I.getPointerOperand();

        pass->PM[&I] = pass->FN.Predicate->getLoadPredicate(
            pass->FN.Term->getValueTerm(lhv),
            pass->FN.Term->getLoadTerm(
                pass->FN.Term->getValueTerm(rhv)
            )
        );
    }

    void visitStoreInst(llvm::StoreInst& I) {
        using llvm::Value;

        Value* lhv = I.getPointerOperand();
        Value* rhv = I.getValueOperand();

        pass->PM[&I] = pass->FN.Predicate->getStorePredicate(
            pass->FN.Term->getValueTerm(lhv),
            pass->FN.Term->getValueTerm(rhv)
        );
    }

    void visitICmpInst(llvm::ICmpInst& I) {
        using llvm::ConditionType;
        using llvm::Value;

        Value* lhv = &I;
        Value* op1 = I.getOperand(0);
        Value* op2 = I.getOperand(1);
        ConditionType cond = conditionType(I.getPredicate());

        pass->PM[&I] = pass->FN.Predicate->getICmpPredicate(
            pass->FN.Term->getValueTerm(lhv),
            pass->FN.Term->getCmpTerm(
                cond,
                pass->FN.Term->getValueTerm(op1),
                pass->FN.Term->getValueTerm(op2)
            )
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
                pass->FN.Term->getTrueTerm()
            );
        pass->TPM[{&I, falseSucc}] =
            pass->FN.Predicate->getBooleanPredicate(
                condTerm,
                pass->FN.Term->getFalseTerm()
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
                    PredicateType::PATH
                );

            cases.push_back(caseTerm);
        }

        const BasicBlock* defaultSucc = I.getDefaultDest();
        pass->TPM[{&I, defaultSucc}] =
            pass->FN.Predicate->getDefaultSwitchCasePredicate(
                condTerm,
                cases
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
            )
        );
    }

    void visitAllocaInst(llvm::AllocaInst& I) {
        using namespace llvm;
        using llvm::Type;

        Value* lhv = &I;
        Value* arraySize = I.getArraySize();
        Type* allocatedType = I.getAllocatedType();

        ASSERTC(isa<ConstantInt>(arraySize));

        unsigned long long numElems = cast<ConstantInt>(arraySize)->getZExtValue();
        numElems = numElems * getTypeSizeInElems(allocatedType);

        ASSERTC(numElems > 0);

        pass->PM[&I] = pass->FN.Predicate->getAllocaPredicate(
            pass->FN.Term->getValueTerm(lhv),
            pass->FN.Term->getIntTerm(numElems)
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
            pass->FN.Term->getGepTerm(rhv, idxs)
        );
    }

    void visitCastInst(llvm::CastInst& I) {
        using namespace llvm;

        Value* lhv = &I;
        Value* rhv = I.getOperand(0);

        Term::Ptr lhvt = pass->FN.Term->getValueTerm(lhv);
        Term::Ptr rhvt = pass->FN.Term->getValueTerm(rhv);

        if (isa<type::Bool>(lhvt->getType()) && ! isa<type::Bool>(rhvt->getType())) {
            rhvt = pass->FN.Term->getCmpTerm(
                ConditionType::NEQ,
                rhvt,
                pass->FN.Term->getIntTerm(0ULL)
            );
        } else if (! isa<type::Bool>(lhvt->getType()) && isa<type::Bool>(rhvt->getType())) {
            lhvt = pass->FN.Term->getCmpTerm(
                ConditionType::NEQ,
                lhvt,
                pass->FN.Term->getIntTerm(0ULL)
            );
        }

        pass->PM[&I] = pass->FN.Predicate->getEqualityPredicate(
            lhvt,
            rhvt
        );
    }

    void visitPHINode(llvm::PHINode& I) {
        using namespace llvm;

        for (unsigned int i = 0; i < I.getNumIncomingValues(); i++) {
            const BasicBlock* from = I.getIncomingBlock(i);
            Value* v = I.getIncomingValue(i);

            pass->PPM[{from, &I}] = pass->FN.Predicate->getEqualityPredicate(
                pass->FN.Term->getValueTerm(&I),
                pass->FN.Term->getValueTerm(v)
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

        pass->PM[&I] = pass->FN.Predicate->getArithPredicate(
            pass->FN.Term->getValueTerm(lhv),
            pass->FN.Term->getBinaryTerm(
                type,
                pass->FN.Term->getValueTerm(op1),
                pass->FN.Term->getValueTerm(op2)
            )
        );
    }

    void visitReturnInst(llvm::ReturnInst& I) {
        using llvm::Value;

        Value* rv = I.getReturnValue();
        if (rv == nullptr) return;

        pass->PM[&I] = pass->FN.Predicate->getEqualityPredicate(
            pass->FN.Term->getReturnValueTerm(I.getParent()->getParent()),
            pass->FN.Term->getValueTerm(rv)
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
    AUX<llvm::TargetData>::addRequiredTransitive(AU);
}

bool DefaultPredicateAnalysis::runOnFunction(llvm::Function& F) {
    init();

    auto* st = GetAnalysis<SlotTrackerPass>::doit(this, F).getSlotTracker(F);
    FN = FactoryNest(st);

    TD = &GetAnalysis<llvm::TargetData>::doit(this, F);

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
