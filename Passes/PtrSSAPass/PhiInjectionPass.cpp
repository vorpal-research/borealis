/*
 * PhiInjectionPass.cpp
 *
 *  Created on: Oct 18, 2012
 *      Author: belyaev
 */



#include "PhiInjectionPass.h"

#include "SLInjectionPass.h"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/CFG.h>


#include <set>
#include <tuple>

namespace {
using namespace llvm;
using namespace borealis;
using namespace borealis::ptrssa;

using borealis::util::view;
using std::set;
using std::pair;
using std::make_pair;
}

#include "intrinsics.h"


void PhiInjectionPass::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequiredTransitive<DominatorTree>();
    AU.addRequiredTransitive<SlotTrackerPass>();
    AU.addPreserved<StoreLoadInjectionPass>();

    // This pass modifies the program, but not the CFG
    AU.setPreservesCFG();
    //AU.setPreservesAll(); // a lie
}

SlotTracker& PhiInjectionPass::getSlotTracker(const Function* F) {
    return *getAnalysis<SlotTrackerPass>().getSlotTracker(F);
}

static bool PhiContainsSource(PHINode* phi, BasicBlock* source) {
    return phi->getBasicBlockIndex(source) != -1;
}

void PhiInjectionPass::propagateInstruction(Instruction& from, Instruction& to, phi_tracker& track) {
    auto parent = from.getParent();
    auto BB = to.getParent();
    auto orig = getOriginOrSelf(&from);

    for(auto succ : view(succ_begin(parent), succ_end(parent))){
        auto key = make_pair(succ, orig);
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

    //if(origins.empty()) mergeOriginInfoFrom(getAnalysis<StoreLoadInjectionPass>());

    phi_tracker track;

    // Iterate over all Basic Blocks of the Function, calling the function that creates sigma functions, if needed
    for (auto& BB : F) {
        for(auto& I : BB) {
            std::vector<User*> uses(I.use_begin(), I.use_end());
            for(User* U : uses) {
                if(auto IUse = dyn_cast<Instruction>(U) ) {
                    if(IUse->getParent() != &BB && !isa<PHINode>(IUse)) {
                        propagateInstruction(I, *IUse, track);
                        auto key = make_pair(IUse->getParent(), getOriginOrSelf(&I));
                        IUse->replaceUsesOfWith(&I, track.at(key));
                    }
                }
            }
        }
//        createNewDefs(BB);
    }

    return false;
}

char PhiInjectionPass::ID;
namespace {
RegisterPass<PhiInjectionPass> X("phi-injection",
        "The pass that places an intrinsic mark on every pointer before every use");
}


