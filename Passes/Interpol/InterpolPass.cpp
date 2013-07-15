/*
 * InterpolPass.cpp
 *
 *  Created on: Jul 15, 2013
 *      Author: Sam Kolton
 */

#include <llvm/Support/InstVisitor.h>

#include "Codegen/intrinsics_manager.h"
#include "Codegen/llvm.h"
#include "Passes/Interpol/InterpolPass.h"
#include "Passes/Tracker/SlotTrackerPass.h"
#include "Solver/Z3Solver.h"
#include "State/PredicateStateBuilder.h"
#include "State/Transformer/AnnotationMaterializer.h"
#include "State/Transformer/CallSiteInitializer.h"

namespace borealis {

////////////////////////////////////////////////////////////////////////////////

class RetInstVisitor : public llvm::InstVisitor<RetInstVisitor> {

public:

    RetInstVisitor(InterpolPass* pass) : pass(pass) {}

    void visitReturnInst(llvm::ReturnInst& RI) {
        /*auto contract = pass->FM->getEns(RI.getParent()->getParent());
        if (contract->isEmpty()) return;

        auto state = pass->PSA->getInstructionState(&RI);
        if (!state) return;

        dbgs() << "Checking: " << RI << endl;
        dbgs() << "  Ensures: " << endl << contract << endl;

        Z3ExprFactory z3ef;
        Z3Solver s(z3ef);

        dbgs() << "  State: " << endl << state << endl;
        if (s.isViolated(contract, state)) {
            pass->DM->addDefect(DefectType::ENS_01, &RI);
        }*/

    	llvm::Value* retValue =  RI.getReturnValue();

    	//if (!retValue) return;
    	//if (!retValue->getType()->isPointerTy()) return;

    	auto state = pass->PSA->getInstructionState(&RI);
    	if (!state) return;

    	Z3ExprFactory z3ef;
		Z3Solver s(z3ef);

		ExecutionContext ctx(z3ef);
		std::string smtlib = state->toZ3(z3ef, &ctx).toSmtLib();
		std::string smtlib_ctx = ctx.toZ3().toSmtLib();

    	dbgs() << "Generating interpolant: " << endl;
    	dbgs() << "     Formula A: " << smtlib << endl;
    	dbgs() << "Execution context: " << endl;
    	dbgs() << smtlib_ctx << endl;
    	//dbgs() << "     Formula B: " <<  << endl;
    	//dbgs() << "     Interpolant: " <<  << endl;
    }

private:

    InterpolPass* pass;

};

////////////////////////////////////////////////////////////////////////////////

InterpolPass::InterpolPass() : ProxyFunctionPass(ID) {}
InterpolPass::InterpolPass(llvm::Pass* pass) : ProxyFunctionPass(ID, pass) {}

void InterpolPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();

    AUX<DefectManager>::addRequiredTransitive(AU);
    AUX<PredicateStateAnalysis>::addRequiredTransitive(AU);
    AUX<SlotTrackerPass>::addRequiredTransitive(AU);
    AUX<DetectNullPass>::addRequiredTransitive(AU);
}

bool InterpolPass::runOnFunction(llvm::Function& F) {

	DM = &GetAnalysis<DefectManager>::doit(this, F);
    PSA = &GetAnalysis<PredicateStateAnalysis>::doit(this, F);
    DNP = &GetAnalysis<DetectNullPass>::doit(this, F);

    auto* slotTracker = GetAnalysis<SlotTrackerPass>::doit(this, F).getSlotTracker(F);
    PF = PredicateFactory::get(slotTracker);
    TF = TermFactory::get(slotTracker);

    PSF = PredicateStateFactory::get();

    RetInstVisitor riv(this);
    riv.visit(F);

    return false;
}

InterpolPass::~InterpolPass() {}

char InterpolPass::ID;
static RegisterPass<InterpolPass>
X("gen-interpol", "Pass generating interpolants");

} /* namespace borealis */
