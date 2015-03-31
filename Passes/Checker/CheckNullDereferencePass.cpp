/*
 * CheckNullDereferencePass.cpp
 *
 *  Created on: Aug 24, 2012
 *      Author: ice-phoenix
 */

#include <llvm/IR/InstVisitor.h>

#include "Passes/Checker/CheckHelper.hpp"
#include "Passes/Checker/CheckNullDereferencePass.h"
#include "State/PredicateStateBuilder.h"

namespace borealis {

////////////////////////////////////////////////////////////////////////////////

class CheckNullsVisitor :
    public llvm::InstVisitor<CheckNullsVisitor>,
    // this is by design (i.e., sharing logging facilities)
    public borealis::logging::ClassLevelLogging<CheckNullDereferencePass> {

private:

    template<class Inst>
    void visitMemoryInst(Inst& I) {
        auto* ptr = I.getPointerOperand();
        if (ptr->isDereferenceablePointer()) return;

        CheckHelper<CheckNullDereferencePass> h(pass, &I, DefectType::INI_03);

        if (h.skip()) return;

        auto q = (
            pass->FN.State *
            pass->FN.Predicate->getInequalityPredicate(
                pass->FN.Term->getValueTerm(ptr),
                pass->FN.Term->getNullPtrTerm()
            )
        )();
        auto ps = pass->PSA->getInstructionState(&I);

        h.check(q, ps);
    }

public:

    CheckNullsVisitor(CheckNullDereferencePass* pass) : pass(pass) {}

    void visitLoadInst(llvm::LoadInst& I) {
        visitMemoryInst(I);
    }

    void visitStoreInst(llvm::StoreInst& I) {
        visitMemoryInst(I);
    }

private:

    CheckNullDereferencePass* pass;

};

////////////////////////////////////////////////////////////////////////////////

CheckNullDereferencePass::CheckNullDereferencePass() : ProxyFunctionPass(ID) {}
CheckNullDereferencePass::CheckNullDereferencePass(llvm::Pass* pass) : ProxyFunctionPass(ID, pass) {}

void CheckNullDereferencePass::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();

    AUX<CheckManager>::addRequiredTransitive(AU);

    AUX<PredicateStateAnalysis>::addRequiredTransitive(AU);
    AUX<llvm::AliasAnalysis>::addRequiredTransitive(AU);
    AUX<DefectManager>::addRequiredTransitive(AU);
    AUX<FunctionManager>::addRequiredTransitive(AU);
    AUX<NameTracker>::addRequiredTransitive(AU);
    AUX<SlotTrackerPass>::addRequiredTransitive(AU);
    AUX<SourceLocationTracker>::addRequiredTransitive(AU);
}

bool CheckNullDereferencePass::runOnFunction(llvm::Function& F) {

    CM = &GetAnalysis<CheckManager>::doit(this, F);
    if (CM->shouldSkipFunction(&F)) return false;

    PSA = &GetAnalysis<PredicateStateAnalysis>::doit(this, F);

    AA = &GetAnalysis<llvm::AliasAnalysis>::doit(this, F);
    DM = &GetAnalysis<DefectManager>::doit(this, F);
    FM = &GetAnalysis<FunctionManager>::doit(this, F);
    NT = &GetAnalysis<NameTracker>::doit(this, F);
    SLT = &GetAnalysis<SourceLocationTracker>::doit(this, F);

    FN = FactoryNest(GetAnalysis<SlotTrackerPass>::doit(this, F).getSlotTracker(F));

    CheckNullsVisitor cnv(this);
    cnv.visit(F);

    return false;
}

CheckNullDereferencePass::~CheckNullDereferencePass() {}

char CheckNullDereferencePass::ID;
static RegisterPass<CheckNullDereferencePass>
X("check-null-deref", "NULL dereference checker");

} /* namespace borealis */
