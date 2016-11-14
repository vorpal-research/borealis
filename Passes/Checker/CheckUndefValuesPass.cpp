/*
 * CheckUndefValuesPass.cpp
 *
 *  Created on: Apr 18, 2013
 *      Author: ice-phoenix
 */

#include <llvm/IR/InstVisitor.h>

#include "Codegen/intrinsics_manager.h"
#include "Passes/Checker/CheckHelper.hpp"
#include "Passes/Checker/CheckUndefValuesPass.h"

namespace borealis {

////////////////////////////////////////////////////////////////////////////////

class UndefInstVisitor : public llvm::InstVisitor<UndefInstVisitor> {

public:

    UndefInstVisitor(CheckUndefValuesPass* pass) : pass(pass) {}

    void visitInstruction(llvm::Instruction& I) {

        // undefs in phis are normal
        if (llvm::isa<llvm::PHINode>(&I)) return;

        auto&& intrinsic_manager = IntrinsicsManager::getInstance();
        if (auto* call = llvm::dyn_cast<llvm::CallInst>(&I)) {
            switch (intrinsic_manager.getIntrinsicType(*call)) {
                case function_type::INTRINSIC_CONSUME:
                case function_type::UNKNOWN: break;
                default: return;
            }
        }

        // FIXME: Better undef propagation analysis

        if (pass->DM->hasDefect(DefectType::NDF_01, &I)) return;

        if (util::viewContainer(I.operands()).any_of(llvm::isaer<llvm::UndefValue>())) {
            pass->DM->addDefect(DefectType::NDF_01, &I);
        }
    }

private:

    CheckUndefValuesPass* pass;

};

////////////////////////////////////////////////////////////////////////////////

CheckUndefValuesPass::CheckUndefValuesPass() : ProxyFunctionPass(ID) {}
CheckUndefValuesPass::CheckUndefValuesPass(llvm::Pass* pass) : ProxyFunctionPass(ID, pass) {}

void CheckUndefValuesPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();

    AUX<CheckManager>::addRequiredTransitive(AU);

    AUX<DefectManager>::addRequiredTransitive(AU);
}

bool CheckUndefValuesPass::runOnFunction(llvm::Function& F) {

    CM = &GetAnalysis<CheckManager>::doit(this, F);
    if (CM->shouldSkipFunction(&F)) return false;

    DM = &GetAnalysis<DefectManager>::doit(this, F);

    UndefInstVisitor uiv(this);
    uiv.visit(F);

    DM->sync();
    return false;
}

CheckUndefValuesPass::~CheckUndefValuesPass() {}

char CheckUndefValuesPass::ID;
static RegisterPass<CheckUndefValuesPass>
X("check-undefs", "Pass that checks for uses of undef values");

} /* namespace borealis */
