/*
 * CheckOutOfBoundsPass.cpp
 *
 *  Created on: Sep 2, 2013
 *      Author: sam
 */

#include <llvm/Support/InstVisitor.h>

#include "Passes/Checker/CheckOutOfBoundsPass.h"
#include "Passes/Tracker/SlotTrackerPass.h"
#include "SMT/MathSAT/Solver.h"
#include "SMT/Z3/Solver.h"
#include "State/PredicateStateBuilder.h"

namespace borealis {

////////////////////////////////////////////////////////////////////////////////


class GepInstVisitor : public llvm::InstVisitor<GepInstVisitor> {

public:

    GepInstVisitor(CheckOutOfBoundsPass* pass) : pass(pass) {}


    void visitGetElementPtrInst(llvm::GetElementPtrInst &GI) {
        if (checkOutOfBounds(GI)) {
            reportOutOfBounds(GI);
        }
    }


private:

    bool checkOutOfBounds(llvm::Instruction& where) {


        PredicateState::Ptr q = (
            pass->FN.State *
            pass->FN.Predicate->getInequalityPredicate(
                    pass->FN.Term->getValueTerm(&where),
                    pass->FN.Term->getInvalidPtrTerm()
            )
        )();

        PredicateState::Ptr ps = pass->PSA->getInstructionState(&where);

        dbgs() << "Query: " << q->toString() << endl;
        dbgs() << "State: " << ps << endl;

#if defined USE_MATHSAT_SOLVER
        MathSAT::ExprFactory ef;
        MathSAT::Solver s(ef);
#else
        Z3::ExprFactory ef;
        Z3::Solver s(ef);
#endif

        if (s.isViolated(q, ps)) {
            dbgs() << "Violated!" << endl;
            return true;
        } else {
            dbgs() << "Passed!" << endl;
            return false;
        }
    }

    void reportOutOfBounds(llvm::Instruction& where) {
        pass->DM->addDefect(DefectType::BUF_05, &where);
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

    AUX<DefectManager>::addRequiredTransitive(AU);
    AUX<PredicateStateAnalysis>::addRequiredTransitive(AU);
    AUX<SlotTrackerPass>::addRequiredTransitive(AU);
}

bool CheckOutOfBoundsPass::runOnFunction(llvm::Function& F) {

    DM = &GetAnalysis<DefectManager>::doit(this, F);
    PSA = &GetAnalysis<PredicateStateAnalysis>::doit(this, F);

    auto* st = GetAnalysis<SlotTrackerPass>::doit(this, F).getSlotTracker(F);
    FN = FactoryNest(st);

    GepInstVisitor giv(this);
    giv.visit(F);
    return false;
}

CheckOutOfBoundsPass::~CheckOutOfBoundsPass() {}

char CheckOutOfBoundsPass::ID;
static RegisterPass<CheckOutOfBoundsPass>
X("check-bounds", "Pass that checks out of bounds");

} /* namespace borealis */
