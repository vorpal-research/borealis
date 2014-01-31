/*
 * CheckUndefValuesPass.cpp
 *
 *  Created on: Apr 18, 2013
 *      Author: ice-phoenix
 */

#include <llvm/Support/InstVisitor.h>

#include "Passes/Checker/CheckHelper.hpp"
#include "Passes/Checker/CheckUndefValuesPass.h"
#include "Codegen/intrinsics_manager.h"

namespace borealis {

////////////////////////////////////////////////////////////////////////////////

class UndefInstVisitor : public llvm::InstVisitor<UndefInstVisitor> {

public:

    UndefInstVisitor(CheckUndefValuesPass* pass) : pass(pass) {}

    void visitInstruction(llvm::Instruction& I) {
        using namespace llvm;
        using borealis::util::view;

        // undefs in phis are normal
        if (isa<PHINode>(&I)) return;

        auto& intrinsic_manager = IntrinsicsManager::getInstance();
        if (auto* call = dyn_cast<CallInst>(&I))
            if (function_type::UNKNOWN != intrinsic_manager.getIntrinsicType(*call))
                return;

        // FIXME: Better undef propagation analysis

        if (pass->DM->hasDefect(DefectType::NDF_01, &I)) return;

        if (view(I.op_begin(), I.op_end()).any_of(isaer<UndefValue>())) {
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

    return false;
}

CheckUndefValuesPass::~CheckUndefValuesPass() {}

char CheckUndefValuesPass::ID;
static RegisterPass<CheckUndefValuesPass>
X("check-undefs", "Pass that checks for uses of undef values");

} /* namespace borealis */
