/*
 * PhiInjectionPass.cpp
 *
 *  Created on: Oct 18, 2012
 *      Author: belyaev
 */

#include <llvm/Support/Casting.h>
#include <llvm/Support/CFG.h>

#include "Passes/Transform/PtrSSAPass/PhiInjectionPass.h"
#include "Passes/Transform/PtrSSAPass/SLInjectionPass.h"

namespace borealis {
namespace ptrssa {

void PhiInjectionPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
    AUX<llvm::DominatorTree>::addRequiredTransitive(AU);
    AUX<StoreLoadInjectionPass>::addPreserved(AU);

    // This pass modifies the program, but not the CFG
    AU.setPreservesCFG();
}

static bool PhiContainsSource(llvm::PHINode* phi, llvm::BasicBlock* source) {
    return phi->getBasicBlockIndex(source) != -1;
}

void PhiInjectionPass::propagateInstruction(llvm::Instruction& from, llvm::Instruction& to, phi_tracker& tracker) {
    using borealis::util::view;

    dbgs() << "Propagating |" << from << "| to |" << to << "|" << endl;

    auto* parent = from.getParent();
    auto* orig = getOriginOrSelf(&from);

    dbgs() << "Origin for them: |" << *orig << "|" << endl;

    for (auto* succ : view(succ_begin(parent), succ_end(parent))) {
        auto key = std::make_pair(succ, orig);
        if (tracker.count(key) && PhiContainsSource(tracker.at(key), parent)) continue;

        llvm::PHINode* phi = nullptr;
        if (tracker.count(key)) {
            phi = tracker.at(key);
        } else {
            auto& BBnext = *succ;

            std::string name;
            if (orig->hasName()) name = (orig->getName() + ".").str();

            phi = llvm::PHINode::Create(from.getType(), 0, name, &BBnext.front());
        }
        phi->addIncoming(&from, from.getParent());

        tracker.insert({key, phi});
        setOrigin(phi, orig);

        propagateInstruction(*phi, to, tracker);
    }
}

bool PhiInjectionPass::runOnFunction(llvm::Function& F) {
    using namespace llvm;

    DT_ = &GetAnalysis<llvm::DominatorTree>::doit(this, F);

    auto sli = getAnalysisIfAvailable<StoreLoadInjectionPass>();
    if (origins.empty() && sli) mergeOriginInfoFrom(*sli);

    phi_tracker tracker;
    for (auto& BB : F) {
        for (auto& I : BB) {
            std::vector<User*> uses(I.use_begin(), I.use_end());
            for (auto* U : uses) {
                if (auto* IUse = dyn_cast<Instruction>(U)) {
                    if (IUse->getParent() != &BB && !isa<PHINode>(IUse)) {
                        propagateInstruction(I, *IUse, tracker);
                        IUse->replaceUsesOfWith(
                                &I,
                                tracker.at({IUse->getParent(), getOriginOrSelf(&I)})
                        );
                    }
                }
            }
        }
    }

    return false;
}

char PhiInjectionPass::ID;
static RegisterPass<PhiInjectionPass>
X("phi-injection", "A pass that injects PHI functions for intrinsically marked pointers");

} // namespace ptrssa
} // namespace borealis
