/*
 * LoopManager.cpp
 *
 *  Created on: Mar 29, 2013
 *      Author: belyaev
 */

#include "Annotation/UnrollAnnotation.h"
#include "Passes/Manager/LoopManager.h"
#include "Passes/Misc/AnnotatorPass.h"
#include "Passes/Tracker/SourceLocationTracker.h"
#include "Util/passes.hpp"
#include "Util/util.h"

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
    using borealis::util::containsKey;

    auto* loopHeader = L->getHeader();

    if (containsKey(data, loopHeader)) {
        return data.at(loopHeader);
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
