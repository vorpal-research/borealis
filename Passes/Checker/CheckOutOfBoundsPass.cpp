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

namespace borealis {

////////////////////////////////////////////////////////////////////////////////

class GepInstVisitor : public llvm::InstVisitor<GepInstVisitor> {

public:

    GepInstVisitor(CheckOutOfBoundsPass* pass) : pass(pass) {}

    void visitGetElementPtrInst(llvm::GetElementPtrInst& GI) {

        CheckHelper<CheckOutOfBoundsPass> h(pass, &GI, DefectType::BUF_01);

        if (h.skip()) return;

        auto q = (
            pass->FN.State *
            pass->FN.Predicate->getInequalityPredicate(
                pass->FN.Term->getValueTerm(&GI),
                pass->FN.Term->getInvalidPtrTerm()
            )
        )();
        auto ps = pass->PSA->getInstructionState(&GI);

        h.check(q, ps);
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

    AA = &GetAnalysis<llvm::AliasAnalysis>::doit(this, F);
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
