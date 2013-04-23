/*
 * LoopManager.cpp
 *
 *  Created on: Mar 29, 2013
 *      Author: belyaev
 */

#include <llvm/Analysis/LoopInfo.h>

#include "Annotation/UnrollAnnotation.h"
#include "Passes/AnnotatorPass.h"
#include "Passes/LoopManager.h"
#include "Passes/SourceLocationTracker.h"
#include "Util/passes.hpp"

namespace borealis {

LoopManager::LoopManager() : llvm::FunctionPass(ID) {}

void LoopManager::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();
    AUX<AnnotatorPass>::addRequiredTransitive(AU);
    AUX<SourceLocationTracker>::addRequiredTransitive(AU);
}

bool LoopManager::runOnFunction(llvm::Function&) {

    using namespace llvm;

    auto& AP = GetAnalysis<AnnotatorPass>::doit(this);
    auto& SL = GetAnalysis<SourceLocationTracker>::doit(this);

    for (auto& anno : AP) {
        if (auto* unroll = dyn_cast<UnrollAnnotation>(anno)) {
            const auto& blocks = SL.getLoopFor(unroll->getLocus());
            // This is generally fucked up, BUT here:
            //   blocks.front() == loop.getHeader()
            if (!blocks.empty()) data[blocks.front()] = unroll->getLevel();
        }
    }

    return false;
}

unsigned LoopManager::getUnrollCount(llvm::Loop* L) const {
    auto it = data.find(L->getHeader());
    if (it != data.end()) {
        return it->second;
    } else {
        return 0;
    }
}

void LoopManager::print(llvm::raw_ostream&, const llvm::Module*) const {
    // TODO: implement print
}

LoopManager::~LoopManager() {}

char LoopManager::ID;
static RegisterPass<LoopManager>
X("loop-manager", "Pass that manages additional loop information (e.g., unroll counts)");

} /* namespace borealis */
