/*
 * DefectManager.cpp
 *
 *  Created on: Jan 24, 2013
 *      Author: ice-phoenix
 */

#include "Passes/DefectManager.h"
#include "Passes/SourceLocationTracker.h"

namespace borealis {

DefectManager::DefectManager() : llvm::ModulePass(ID) {}

void DefectManager::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();
    AU.addRequiredTransitive<SourceLocationTracker>();
}

void DefectManager::addDefect(DefectType type, llvm::Instruction* where) {
    auto* locs = &getAnalysis<SourceLocationTracker>();
    data.insert({type, locs->getLocFor(where)});
}

char DefectManager::ID;
static llvm::RegisterPass<DefectManager>
X("defect-manager", "Pass that collects and filters detected defects");

} /* namespace borealis */
