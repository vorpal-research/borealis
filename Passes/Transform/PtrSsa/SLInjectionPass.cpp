/*
 * SLInjectionPass.cpp
 *
 *  Created on: Oct 18, 2012
 *      Author: belyaev
 */

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Type.h>

#include <iterator>

#include "Codegen/intrinsics_manager.h"
#include "Passes/Transform/PtrSsa/PhiInjectionPass.h"
#include "Passes/Transform/PtrSsa/SLInjectionPass.h"

namespace borealis {
namespace ptrssa {

llvm::Function* StoreLoadInjectionPass::createNuevoFunc(
        llvm::Type* pointed, llvm::Module* daModule) {
    auto* ptr = llvm::PointerType::getUnqual(pointed);
    auto* ftype = llvm::FunctionType::get(ptr, ptr, false /* isVarArg */);
    return IntrinsicsManager::getInstance().createIntrinsic(
            function_type::INTRINSIC_PTR_VERSION,
            borealis::util::toString(*pointed), // FIXME: use TypePrinting or SlotTrackerPass
            ftype,
            daModule);
}

void StoreLoadInjectionPass::createNewDefs(llvm::BasicBlock& BB) {
    for (auto& I : BB) {
        checkAndUpdatePtrs<llvm::StoreInst>(&I);
        checkAndUpdatePtrs<llvm::LoadInst>( &I);
        checkAndUpdatePtrs<llvm::CallInst>( &I);
    }
}

void StoreLoadInjectionPass::renameNewDefs(
        llvm::Instruction* newdef,
        llvm::Value* Op) {

    using borealis::util::view;

    // This vector of Instruction* points to the uses of V.
    // It is used because the use_iterators
    // are invalidated when we do the renaming
    std::vector<llvm::Instruction*> uses;
    uses.reserve(Op->getNumUses());
    for (auto* use : view(Op->user_begin(), Op->user_end()))
        uses.push_back(llvm::dyn_cast<llvm::Instruction>(use));

    llvm::BasicBlock* BB = newdef->getParent();
    llvm::BasicBlock::iterator newdefIt(newdef);
    int newdefDist = std::distance(BB->begin(), newdefIt);

    for (auto* use : uses) {
        if (use == nullptr) continue;
        if (use == newdef) continue;

        llvm::BasicBlock* useBB = use->getParent();
        llvm::BasicBlock::iterator useIt(use);

        // Check if use is in the dominator tree of newdef(V)
        if (DT_->dominates(BB, useBB)) {
            // If they are in the same basic block,
            // rename only if use is *after* the newdef
            if (BB == useBB) {
                int useDist = std::distance(BB->begin(), useIt);
                if (useDist > newdefDist) {
                    use->replaceUsesOfWith(Op, newdef);
                }
            } else {
                use->replaceUsesOfWith(Op, newdef);
            }
        }
    }
}

void StoreLoadInjectionPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
    AUX<llvm::DominatorTreeWrapperPass>::addRequiredTransitive(AU);
    AUX<PhiInjectionPass>::addPreserved(AU);

    // This pass modifies the program, but not the CFG
    AU.setPreservesCFG();
}

bool StoreLoadInjectionPass::runOnBasicBlock(llvm::BasicBlock& bb) {
    DT_ = &GetAnalysis<llvm::DominatorTreeWrapperPass>::doit(this, *bb.getParent()).getDomTree();

    auto phii = getAnalysisIfAvailable<PhiInjectionPass>();

    if (origins.empty() && phii) mergeOriginInfoFrom(*phii);

    createNewDefs(bb);

    return false;
}

char StoreLoadInjectionPass::ID;
static RegisterPass<StoreLoadInjectionPass>
X("sl-injection", "Inject an intrinsic for every store and load");

} // namespace ptrssa
} // namespace borealis
