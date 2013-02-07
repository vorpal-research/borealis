/*
 * PhiInjectionPass.cpp
 *
 *  Created on: Oct 18, 2012
 *      Author: belyaev
 */

#include <llvm/ADT/ArrayRef.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/CFG.h>

#include <set>
#include <tuple>

#include "Codegen/intrinsics.h"
#include "PhiInjectionPass.h"
#include "SLInjectionPass.h"

namespace borealis {
namespace ptrssa {

using namespace llvm;

void PhiInjectionPass::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequiredTransitive<DominatorTree>();
    AU.addRequiredTransitive<SlotTrackerPass>();
    AU.addPreserved<StoreLoadInjectionPass>();

    // This pass modifies the program, but not the CFG
    AU.setPreservesCFG();
}

static bool PhiContainsSource(PHINode* phi, BasicBlock* source) {
    return phi->getBasicBlockIndex(source) != -1;
}

void PhiInjectionPass::propagateInstruction(Instruction& from, Instruction& to, phi_tracker& track) {
    using borealis::util::view;

    infos() << "Propagating |" << from << "| to |" << to << "|" << endl;

    auto parent = from.getParent();
    auto orig = getOriginOrSelf(&from);

    infos() << "Origin for them: |" << *orig << "| " << endl << endl;

    for(auto succ : view(succ_begin(parent), succ_end(parent))){
        auto key = std::make_pair(succ, orig);
        if(track.count(key) && PhiContainsSource(track.at(key), parent)) continue;

        PHINode* phi = nullptr;
        if(track.count(key)) {
            phi = track.at(key);
        } else {
            auto& BBnext = *succ;

            std::string name;
            if(orig->hasName()) name = (orig->getName() + ".").str();

            phi = PHINode::Create(from.getType(), 0, name, &BBnext.front());
        }
        phi->addIncoming(&from, from.getParent());

        track.insert(make_pair(key, phi));
        setOrigin(phi, orig);

        propagateInstruction(*phi, to, track);
    }
}

bool PhiInjectionPass::runOnFunction(Function &F) {
    DT_ = &getAnalysis<DominatorTree>();

    auto sli = getAnalysisIfAvailable<StoreLoadInjectionPass>();
    if(origins.empty() && sli) mergeOriginInfoFrom(*sli);

    phi_tracker track;
    for (auto& BB : F) {
        for (auto& I : BB) {
            std::vector<User*> uses(I.use_begin(), I.use_end());
            for (User* U : uses) {
                if (auto IUse = dyn_cast<Instruction>(U) ) {
                    if(IUse->getParent() != &BB && !isa<PHINode>(IUse)) {
                        propagateInstruction(I, *IUse, track);
                        auto key = std::make_pair(IUse->getParent(), getOriginOrSelf(&I));
                        IUse->replaceUsesOfWith(&I, track.at(key));
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
