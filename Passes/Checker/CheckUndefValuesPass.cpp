/*
 * CheckUndefValuesPass.cpp
 *
 *  Created on: Apr 18, 2013
 *      Author: ice-phoenix
 */

#include <llvm/Support/InstVisitor.h>

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

        auto& intrinsic_manager = borealis::IntrinsicsManager::getInstance();
        if (auto* call = dyn_cast<llvm::CallInst>(&I))
            if (intrinsic_manager.getIntrinsicType(*call) != function_type::UNKNOWN)
                return;

        for (Value* op : view(I.op_begin(), I.op_end())) {
            if (isa<UndefValue>(op)) {
                pass->DM->addDefect(DefectType::NDF_01, &I);
                break;
            }
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

    AUX<DefectManager>::addRequiredTransitive(AU);
}

bool CheckUndefValuesPass::runOnFunction(llvm::Function& F) {

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
