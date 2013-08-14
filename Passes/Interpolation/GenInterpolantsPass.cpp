/*
 * GenInterpolantsPass.cpp
 *
 *  Created on: Aug 14, 2013
 *      Author: sam
 */

#include <llvm/Support/InstVisitor.h>

#include "Passes/Interpolation/GenInterpolantsPass.h"
#include "SMT/MathSAT/Solver.h"
#include "State/PredicateStateBuilder.h"

namespace borealis {


class RetInstVisitor :
    public llvm::InstVisitor<RetInstVisitor>,
    public borealis::logging::ClassLevelLogging<RetInstVisitor> {

public:

	RetInstVisitor(GenInterpolantsPass* pass) : pass(pass) {}

    void visitReturnInst(llvm::ReturnInst& I) {
		using llvm::Value;

		Value* ret = I.getReturnValue();
		if (ret != nullptr  &&  ret->getType()->isPointerTy()) {
			generateInterpolant(I, *ret);
		}
    }

    mathsat::Expr generateInterpolant(
            llvm::Instruction& where,
            llvm::Value& what) {

    	MathSAT::ExprFactory msatef;

    	PredicateState::Ptr query = (
            pass->FN.State *
//            pass->FN.Predicate->getInequalityPredicate(
			pass->FN.Predicate->getEqualityPredicate(
                pass->FN.Term->getValueTerm(&what),
                pass->FN.Term->getNullPtrTerm()
            )
        )();

    	PredicateState::Ptr state = pass->PSA->getInstructionState(&where);
		if (!state) {
			return msatef.unwrap().bool_val(false);
		}

        dbgs() << "Generating interpolant: " << endl
               << "Ret: " << what << endl
               << "Query: " << query << endl
				<< "State: " << state << endl;

        MathSAT::Solver s(msatef);
		auto interpol = s.getInterpolant(query, state);
		dbgs()  << "Interpolant: " << endl
				<< interpol;
		return interpol;
    }

private:
    GenInterpolantsPass* pass;

};

////////////////////////////////////////////////////////////////////////////////

GenInterpolantsPass::GenInterpolantsPass() : ProxyFunctionPass(ID) {}
GenInterpolantsPass::GenInterpolantsPass(llvm::Pass* pass) : ProxyFunctionPass(ID, pass) {}

void GenInterpolantsPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();

    AUX<FunctionManager>::addRequiredTransitive(AU);
    AUX<PredicateStateAnalysis>::addRequiredTransitive(AU);
    AUX<SlotTrackerPass>::addRequiredTransitive(AU);
}

bool GenInterpolantsPass::runOnFunction(llvm::Function& F) {
    FM = &GetAnalysis<FunctionManager>::doit(this, F);
	PSA = &GetAnalysis<PredicateStateAnalysis>::doit(this, F);

    auto* st = GetAnalysis<SlotTrackerPass>::doit(this, F).getSlotTracker(F);
    FN = FactoryNest(st);

    RetInstVisitor ret(this);
    ret.visit(F);

    return false;
}

GenInterpolantsPass::~GenInterpolantsPass() {}

char GenInterpolantsPass::ID;
static RegisterPass<GenInterpolantsPass>
X("gen-interpolans", "Pass generating interpolants");

} /* namespace borealis */
