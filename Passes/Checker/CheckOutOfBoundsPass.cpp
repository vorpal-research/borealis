/*
 * CheckOutOfBoundsPass.cpp
 *
 *  Created on: Sep 2, 2013
 *      Author: sam
 */

#include <llvm/Support/InstVisitor.h>

#include "Codegen/intrinsics_manager.h"
#include "Codegen/llvm.h"
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
        auto spliters = filterArrays(splitGep(GI));
        for (auto spliter: spliters) {
            auto numElements = spliter.first->getArrayNumElements();
            if (checkOutOfBounds(GI, *spliter.second, numElements)) {
                reportOutOfBounds(GI);
            }
        }
    }

    std::vector< std::pair<llvm::Type*, llvm::Value*> > splitGep(const llvm::GetElementPtrInst &GI) {
        using namespace llvm;
        auto ptrType = GI.getPointerOperand()->getType();

        std::vector< std::pair<llvm::Type*, llvm::Value*> > spliters;
        spliters.reserve(GI.getNumIndices());
        spliters.emplace_back(ptrType, GI.getOperand(1));

        std::vector<llvm::Value *> indicies;
        indicies.reserve(GI.getNumIndices());
        auto endIter = GI.idx_end();
        auto opIter = GI.idx_begin();
        ++opIter;
        for (; opIter != endIter; ++opIter) {
            auto type = GetElementPtrInst::getIndexedType(ptrType, indicies);
            spliters.emplace_back(type, *opIter);
            indicies.push_back(*opIter);
        }
        return spliters;
    }

    std::vector< std::pair<llvm::ArrayType*, llvm::Value*> > filterArrays(
            const std::vector< std::pair<llvm::Type*, llvm::Value*> > &splitters) {
        auto filter = [](const std::pair<llvm::Type*, llvm::Value*> &pair) -> bool {
            return pair.first->isArrayTy();
        };
        std::vector< std::pair<llvm::Type*, llvm::Value*> > copy;
        std::copy_if(splitters.begin(), splitters.end(), std::back_inserter(copy), filter);
        std::vector< std::pair<llvm::ArrayType*, llvm::Value*> > arrays;
        for (auto filtered: copy) {
            auto arrType = llvm::dyn_cast<llvm::ArrayType>(filtered.first);
            arrays.emplace_back(arrType, filtered.second);
        }
        return arrays;
    }


    bool checkOutOfBounds(
            llvm::Instruction& where,
            llvm::Value& what,
            unsigned howMuch) {


        PredicateState::Ptr q = (
            pass->FN.State *
            pass->FN.Predicate->getEqualityPredicate(
                pass->FN.Term->getCmpTerm(llvm::ConditionType::LT,
                                          pass->FN.Term->getValueTerm(&what),
                                          pass->FN.Term->getIntTerm(howMuch)
                ),
                pass->FN.Term->getTrueTerm()
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
        pass->DM->addDefect("Out of bounds", &where);
    }


private:

    CheckOutOfBoundsPass* pass;

};

////////////////////////////////////////////////////////////////////////////////

CheckOutOfBoundsPass::CheckOutOfBoundsPass() : ProxyFunctionPass(ID) {}
CheckOutOfBoundsPass::CheckOutOfBoundsPass(llvm::Pass* pass) : ProxyFunctionPass(ID, pass) {}

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
