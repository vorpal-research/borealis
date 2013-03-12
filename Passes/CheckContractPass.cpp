/*
 * CheckContractPass.cpp
 *
 *  Created on: Feb 27, 2013
 *      Author: ice-phoenix
 */

#include <llvm/Support/InstVisitor.h>

#include "Logging/logger.hpp"
#include "Passes/CheckContractPass.h"
#include "Solver/Z3Solver.h"
#include "State/CallSiteInitializer.h"
#include "State/PredicateState.h"
#include "Util/passes.hpp"

namespace borealis {

////////////////////////////////////////////////////////////////////////////////

class CallInstVisitor : public llvm::InstVisitor<CallInstVisitor> {

public:

    CallInstVisitor(CheckContractPass* pass) : pass(pass) {}

    void visitCallInst(llvm::CallInst& CI) {
        auto& contract = pass->FM->get(CI, pass->PF.get(), pass->TF.get());
        auto& states = pass->PSA->getPredicateStateMap()[&CI];

        auto requires = contract.filterByTypes({PredicateType::REQUIRES});
        if (requires.isEmpty()) return;

        CallSiteInitializer csi(CI, pass->TF.get());
        PredicateState instantiatedRequires = requires.map(
            [&csi](Predicate::Ptr p) {
                return csi.transform(p);
            }
        );

        dbgs() << "Checking: " << CI << endl;
        dbgs() << "  Requires: " << endl << instantiatedRequires << endl;

        z3::context ctx;
        Z3ExprFactory z3ef(ctx);
        Z3Solver s(z3ef);

        for (auto& state : states) {
            dbgs() << "  State: " << endl << state << endl;
            if (s.checkViolated(instantiatedRequires, state)) {
                pass->DM->addDefect(DefectType::REQ_01, &CI);
            }
        }
    }

    void visitReturnInst(llvm::ReturnInst& RI) {
        auto& contract = pass->FM->get(RI.getParent()->getParent());
        auto& states = pass->PSA->getPredicateStateMap()[&RI];

        auto ensures = contract.filterByTypes({PredicateType::ENSURES});
        if (ensures.isEmpty()) return;

        dbgs() << "Checking: " << RI << endl;
        dbgs() << "  Ensures: " << endl << ensures << endl;

        z3::context ctx;
        Z3ExprFactory z3ef(ctx);
        Z3Solver s(z3ef);

        for (auto& state : states) {
            dbgs() << "  State: " << endl << state << endl;
            if (s.checkViolated(ensures, state)) {
                pass->DM->addDefect(DefectType::ENS_01, &RI);
            }
        }
    }

private:

    CheckContractPass* pass;

};

////////////////////////////////////////////////////////////////////////////////

CheckContractPass::CheckContractPass() : ProxyFunctionPass(ID) {}
CheckContractPass::CheckContractPass(llvm::Pass* pass) : ProxyFunctionPass(ID, pass) {}

void CheckContractPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();

    AUX<DefectManager>::addRequiredTransitive(AU);
    AUX<FunctionManager>::addRequiredTransitive(AU);
    AUX<PredicateStateAnalysis>::addRequiredTransitive(AU);
    AUX<SlotTrackerPass>::addRequiredTransitive(AU);
}

bool CheckContractPass::runOnFunction(llvm::Function& F) {

    DM = &GetAnalysis<DefectManager>::doit(this, F);
    FM = &GetAnalysis<FunctionManager>::doit(this, F);
    PSA = &GetAnalysis<PredicateStateAnalysis>::doit(this, F);

    SlotTracker* slotTracker = GetAnalysis<SlotTrackerPass>::doit(this, F).getSlotTracker(F);
    PF = PredicateFactory::get(slotTracker);
    TF = TermFactory::get(slotTracker);

    CallInstVisitor civ(this);
    civ.visit(F);

    return false;
}

CheckContractPass::~CheckContractPass() {}

char CheckContractPass::ID;
static RegisterPass<CheckContractPass>
X("check-contracts", "Pass that checks annotated code contracts");

} /* namespace borealis */
