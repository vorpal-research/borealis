/*
 * CheckManager.cpp
 *
 *  Created on: Dec 9, 2013
 *      Author: ice-phoenix
 */

#include "Config/config.h"
#include "Passes/Checker/CheckManager.h"
#include "Util/passes.hpp"

namespace borealis {

CheckManager::CheckManager() : llvm::ImmutablePass(ID) {}

void CheckManager::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();
}

void CheckManager::initializePass() {
    static config::MultiConfigEntry includesOpt("checkers", "includes");
    static config::MultiConfigEntry excludesOpt("checkers", "excludes");

    includes.insert(includesOpt.begin(), includesOpt.end());
    excludes.insert(excludesOpt.begin(), excludesOpt.end());
}

bool CheckManager::shouldSkipFunction(llvm::Function* F) const {

    auto fName = F->getName().str();

    if (excludes.count(fName) > 0) return true;
    if (includes.empty()) return false;
    if (includes.count(fName) > 0) return false;
    return true;
}

CheckManager::~CheckManager() {}

char CheckManager::ID;
static RegisterPass<CheckManager>
X("check-manager", "Pass that manages other checker passes");

} /* namespace borealis */
