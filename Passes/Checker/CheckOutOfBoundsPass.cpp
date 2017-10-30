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

    std::unordered_set<llvm::Value*> visited;
    std::unordered_map<std::pair<llvm::Value*, llvm::Value*>, llvm::Instruction*> overApp;

public:

    GepInstVisitor(CheckOutOfBoundsPass* pass) : pass(pass) {}

    void visitGEPOperator(llvm::Instruction& loc, llvm::GEPOperator& GI) {
        if(visited.count(&GI)) return;
        visited.insert(&GI);

        CheckHelper<CheckOutOfBoundsPass> h(pass, &loc, DefectType::BUF_01);

        if (isTriviallyInboundsGEP(&GI)) return;
        if (GI.isDereferenceablePointer(loc.getDataLayout())) return;

        if (h.skip()) return;

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
        auto ps = pass->getInstructionState(&loc);

        h.check(q, ps);
    }

    void visitGetElementPtrInst(llvm::GetElementPtrInst& GI) {
        visitGEPOperator(GI, llvm::cast<llvm::GEPOperator>(GI));
    }

    void visitInstruction(llvm::Instruction& I) {

        for (auto&& op : util::viewContainer(I.operands())
                         .map(llvm::dyn_caster<llvm::GEPOperator>())
                         .filter()) {
            visitGEPOperator(I, *op);
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
    //PSA = &GetAnalysis<PredicateStateAnalysis>::doit(this, F);
    PSA = nullptr;

    ST = &GetAnalysis<SlotTrackerPass>::doit(this, F);
    FN = FactoryNest(F.getDataLayout(), ST->getSlotTracker(F));

    GepInstVisitor giv(this);
    giv.visit(F);

    DM->sync();
    return false;
}

PredicateState::Ptr CheckOutOfBoundsPass::getInstructionState(llvm::Instruction* I) {
    auto F = I->getParent()->getParent();
    if(!PSA) PSA = &GetAnalysis<PredicateStateAnalysis>::doit(this, *F);
    return PSA->getInstructionState(I);
}

CheckOutOfBoundsPass::~CheckOutOfBoundsPass() {}

char CheckOutOfBoundsPass::ID;
static RegisterPass<CheckOutOfBoundsPass>
X("check-bounds", "Pass that checks out of bounds");

} /* namespace borealis */
