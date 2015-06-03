/*
 * CheckOutOfBoundsPass.cpp
 *
 *  Created on: Sep 2, 2013
 *      Author: sam
 */

#include <llvm/IR/InstVisitor.h>

#include "Passes/Checker/CheckHelper.hpp"
#include "Passes/Checker/CheckOutOfBoundsPass.h"
#include "Passes/Tracker/SlotTrackerPass.h"
#include "State/PredicateStateBuilder.h"
#include "Term/TermBuilder.h"

namespace borealis {

////////////////////////////////////////////////////////////////////////////////

class GepInstVisitor : public llvm::InstVisitor<GepInstVisitor> {

public:

    GepInstVisitor(CheckOutOfBoundsPass* pass) : pass(pass) {}

    void visitGetElementPtrInst(llvm::GetElementPtrInst& GI) {

        CheckHelper<CheckOutOfBoundsPass> h(pass, &GI, DefectType::BUF_01);

        if (h.skip()) return;

        if (isTriviallyInboundsGEP(&GI)) return;

        auto shift = (
                        pass->FN.Term *
                        pass->FN.Term->getValueTerm(&GI)
                        -
                        pass->FN.Term->getValueTerm(GI.getPointerOperand())
                    );

        auto q = (
            pass->FN.State *
            pass->FN.Predicate->getEqualityPredicate(
                pass->FN.Term->getCmpTerm(
                    llvm::ConditionType::UGT,
                    pass->FN.Term->getBoundTerm(pass->FN.Term->getValueTerm(GI.getPointerOperand())),
                    shift
                ),
                pass->FN.Term->getTrueTerm()
            )
        )();
        auto ps = pass->PSA->getInstructionState(&GI);

        h.check(q, ps);
    }

    void visitInstruction(llvm::Instruction& I) {

        for (auto&& op : util::viewContainer(I.operands())
                         .map(llvm::dyn_caster<llvm::ConstantExpr>())
                         .filter()
                         .filter([](auto&& op_) { return llvm::Instruction::GetElementPtr == op_->getOpcode(); })) {

            CheckHelper<CheckOutOfBoundsPass> h(pass, &I, DefectType::BUF_01);

            if (h.skip()) return;

            if (isTriviallyInboundsGEP(op)) return;

            auto shift = (
                pass->FN.Term *
                pass->FN.Term->getValueTerm(op)
                -
                pass->FN.Term->getValueTerm(op->getOperand(0))
            );

            auto q = (
                pass->FN.State *
                pass->FN.Predicate->getEqualityPredicate(
                    pass->FN.Term->getCmpTerm(
                        llvm::ConditionType::UGT,
                        pass->FN.Term->getBoundTerm(pass->FN.Term->getValueTerm(op->getOperand(0))),
                        shift
                    ),
                    pass->FN.Term->getTrueTerm()
                )
            )();
            auto ps = pass->PSA->getInstructionState(&I);

            h.check(q, ps);
        }
    }

private:

    CheckOutOfBoundsPass* pass;

};

////////////////////////////////////////////////////////////////////////////////

CheckOutOfBoundsPass::CheckOutOfBoundsPass() : ProxyFunctionPass(ID) {}
CheckOutOfBoundsPass::CheckOutOfBoundsPass(llvm::Pass* pass) :
        ProxyFunctionPass(ID, pass) {}

void CheckOutOfBoundsPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();

    AUX<CheckManager>::addRequiredTransitive(AU);

    AUX<llvm::AliasAnalysis>::addRequiredTransitive(AU);
    AUX<DefectManager>::addRequiredTransitive(AU);
    AUX<FunctionManager>::addRequiredTransitive(AU);
    AUX<PredicateStateAnalysis>::addRequiredTransitive(AU);
    AUX<SlotTrackerPass>::addRequiredTransitive(AU);
}

bool CheckOutOfBoundsPass::runOnFunction(llvm::Function& F) {

    CM = &GetAnalysis<CheckManager>::doit(this, F);
    if (CM->shouldSkipFunction(&F)) return false;

    AA = getAnalysisIfAvailable<llvm::AliasAnalysis>();
    DM = &GetAnalysis<DefectManager>::doit(this, F);
    FM = &GetAnalysis<FunctionManager>::doit(this, F);
    PSA = &GetAnalysis<PredicateStateAnalysis>::doit(this, F);

    FN = FactoryNest(GetAnalysis<SlotTrackerPass>::doit(this, F).getSlotTracker(F));

    GepInstVisitor giv(this);
    giv.visit(F);

    return false;
}

CheckOutOfBoundsPass::~CheckOutOfBoundsPass() {}

char CheckOutOfBoundsPass::ID;
static RegisterPass<CheckOutOfBoundsPass>
X("check-bounds", "Pass that checks out of bounds");

} /* namespace borealis */
