/*
 * GenInterpolantsPass.cpp
 *
 *  Created on: Aug 14, 2013
 *      Author: sam
 */

#include <llvm/Support/InstVisitor.h>

#include <memory>

#include "Passes/Interpolation/GenInterpolantsPass.h"
#include "Passes/Tracker/SlotTrackerPass.h"
#include "SMT/MathSAT/Solver.h"
#include "SMT/MathSAT/Unlogic/Unlogic.h"
#include "State/PredicateStateBuilder.h"

namespace borealis {

class RetInstVisitor :
        public llvm::InstVisitor<RetInstVisitor>,
        // this is by design (i.e., sharing logging facilities)
        public borealis::logging::ClassLevelLogging<GenInterpolantsPass> {

    USING_SMT_IMPL(MathSAT);

public:

    RetInstVisitor(GenInterpolantsPass* pass) : pass(pass) {}

    void visitReturnInst(llvm::ReturnInst& I) {
        using borealis::mathsat_::unlogic::undoThat;
        using llvm::Value;

        auto* ret = I.getReturnValue();
        if (ret == nullptr || ret->getType()->isPointerTy()) return;

        auto ITP = generateInterpolant(I, *ret);
        auto* F = I.getParent()->getParent();
        dbgs() << "Updating function: " << F->getName() << endl
               << "from: " << endl
               << pass->FM->getBdy(F) << endl;

        pass->FM->update(F, undoThat(ITP));

        dbgs() << "to: " << endl
               << pass->FM->getBdy(F) << endl;
    }

    Dynamic generateInterpolant(
            llvm::Instruction& where,
            llvm::Value& what) {

        ExprFactory msatef;

        PredicateState::Ptr query = (
            pass->FN.State *
            pass->FN.Predicate->getEqualityPredicate(
                pass->FN.Term->getValueTerm(&what),
                pass->FN.Term->getNullPtrTerm()
            )
        )();

        PredicateState::Ptr state = pass->PSA->getInstructionState(&where);
        if (!state) return msatef.getTrue();

        dbgs() << "Generating interpolant: " << endl
               << "  At: " << what << endl
               << "  Query: " << query << endl
               << "  State: " << state << endl;

        Solver s(msatef);

        auto interpol = s.getInterpolant(query, state);
        dbgs()  << "Interpolant: " << endl
                << interpol << endl;
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
    if ( ! doInterpolation() ) return false;

    FM = &GetAnalysis<FunctionManager>::doit(this, F);
    PSA = &GetAnalysis<PredicateStateAnalysis>::doit(this, F);

    auto* st = GetAnalysis<SlotTrackerPass>::doit(this, F).getSlotTracker(F);
    FN = FactoryNest(st);

    RetInstVisitor ret(this);
    ret.visit(F);

    return false;
}

bool GenInterpolantsPass::doInterpolation() {
    static config::ConfigEntry<bool> interpol("interpolation", "enable-interpolation");
    return interpol.get(false);
}

GenInterpolantsPass::~GenInterpolantsPass() {}

////////////////////////////////////////////////////////////////////////////////

char GenInterpolantsPass::ID;
static RegisterPass<GenInterpolantsPass>
X("gen-interpolants", "Pass generating interpolants");

} /* namespace borealis */
