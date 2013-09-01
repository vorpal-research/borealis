/*
 * NameTracker.cpp
 *
 *  Created on: Oct 11, 2012
 *      Author: belyaev
 */

#include "Passes/Tracker/NameTracker.h"
#include "Passes/Tracker/SlotTrackerPass.h"
#include "Util/passes.hpp"

namespace borealis {

void addName(SlotTracker*, llvm::Value& V, NameTracker::nameResolver_t& resolver) {
    if (V.hasName()) {
        resolver[V.getName().str()] = &V;
    } else if (auto* RI = llvm::dyn_cast<llvm::ReturnInst>(&V)) {
        auto* F = RI->getParent()->getParent();
        resolver["\\result_" + F->getName().str()] = &V;
    }
}

void NameTracker::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();
    AUX<SlotTrackerPass>::addRequiredTransitive(AU);
}

bool NameTracker::runOnModule(llvm::Module& M) {

    auto* st = GetAnalysis<SlotTrackerPass>::doit(this).getSlotTracker(M);
    auto& resolver = globalResolver;

    for (auto& G : M.getGlobalList()) {
        addName(st, G, resolver);
    }

    for (auto& F : M) {

        auto* st = GetAnalysis<SlotTrackerPass>::doit(this, F).getSlotTracker(F);
        auto& resolver = localResolvers[&F];

        addName(st, F, resolver);
        for (auto& A : F.getArgumentList()) {
            addName(st, A, resolver);
        }
        for (auto& BB : F) {
            for (auto& I : BB) {
                addName(st, I, resolver);
            }
        }
    }

    return false;
}

void NameTracker::print(llvm::raw_ostream&, const llvm::Module*) const {
    auto dbg = dbgs();

    dbg << "Global resolver:" << endl;
    dbg << globalResolver << endl;

    for (const auto& lr : localResolvers) {
        dbg << "Local resolver for " << lr.first->getName().str() << ":" << endl;
        dbg << lr.second << endl;
    }
}

char NameTracker::ID;
static RegisterPass<NameTracker>
X("name-tracker", "Pass used to track value names in LLVM IR");

} // namespace borealis
