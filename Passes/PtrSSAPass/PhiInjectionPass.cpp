/*
 * PhiInjectionPass.cpp
 *
 *  Created on: Oct 18, 2012
 *      Author: belyaev
 */

#include "PhiInjectionPass.h"

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
using std::tuple;
using std::make_tuple;
}

#include "intrinsics.h"


void PhiInjectionPass::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequiredTransitive<DominatorTree>();
    AU.addRequiredTransitive<SlotTrackerPass>();

    // This pass modifies the program, but not the CFG
    AU.setPreservesCFG();
}

SlotTracker& PhiInjectionPass::getSlotTracker(const Function* F) {
    return *getAnalysis<SlotTrackerPass>().getSlotTracker(F);
}

void PhiInjectionPass::propagateInstruction(Instruction& from, Instruction& to, phi_tracker& track) {
    auto parent = from.getParent();
    auto BB = to.getParent();
    if(parent == BB) return;

    auto orig = getOriginOrSelf(&from);
    for(auto succ : view(succ_begin(parent), succ_end(parent))){
        if(track.count(make_tuple(parent, succ, orig))) continue;

        auto& BBnext = *succ;

        std::string name;
        if(orig->hasName()) name = orig->getName();

        auto phi = PHINode::Create(from.getType(), 0, name, &BBnext.front());
        phi->addIncoming(&from, from.getParent());

        track.insert(make_tuple(parent, succ, orig));
        setOrigin(phi, orig);

        to.replaceUsesOfWith(&from, phi);

        propagateInstruction(*phi, to, track);
    }

}

bool PhiInjectionPass::runOnFunction(Function &F) {
    DT_ = &getAnalysis<DominatorTree>();

    phi_tracker track;

    // Iterate over all Basic Blocks of the Function, calling the function that creates sigma functions, if needed
    for (auto& BB : F) {
        for(auto& I : BB) {
            for(auto U : view(I.use_begin(), I.use_end())) {
                if(auto Ch = dyn_cast<Instruction>(U) ) {
                    if(Ch->getParent() != &BB) {
                        propagateInstruction(I, *Ch, track);
                    }
                }
            }
        }
//        createNewDefs(BB);
    }

    return false;
}


char PhiInjectionPass::ID = 0;
static RegisterPass<PhiInjectionPass> X("phi-injection",
        "The pass that places an intrinsic mark on every pointer before every use");



