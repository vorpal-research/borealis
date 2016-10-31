/*
 * CheckManager.cpp
 *
 *  Created on: Dec 9, 2013
 *      Author: ice-phoenix
 */

#include "Codegen/intrinsics_manager.h"
#include "Config/config.h"
#include "Passes/Checker/CheckManager.h"
#include "Util/passes.hpp"
#include "Passes/Checker/CallGraphSlicer.h"

namespace borealis {

CheckManager::CheckManager() : llvm::ModulePass(ID) {}

void CheckManager::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();
    AUX<CallGraphSlicer>::addRequiredTransitive(AU);
}

void CheckManager::initializePass() {
    static config::MultiConfigEntry includesOpt("checkers", "includes");
    static config::MultiConfigEntry excludesOpt("checkers", "excludes");

    includes.insert(includesOpt.begin(), includesOpt.end());
    excludes.insert(excludesOpt.begin(), excludesOpt.end());
}

bool CheckManager::shouldSkipFunction(llvm::Function* F) const {
    auto&& cgs = GetAnalysis<CallGraphSlicer>::doit(this);
    if(cgs.doSlicing() && !cgs.getSlice().count(F)) return true;

    IntrinsicsManager& im = IntrinsicsManager::getInstance();

    if (function_type::UNKNOWN != im.getIntrinsicType(F)) return true;

    auto fName = F->getName().str();

    if (excludes.count(fName) > 0) return true;
    if (includes.empty()) return false;
    if (includes.count(fName) > 0) return false;
    return true;
}

bool CheckManager::shouldSkipInstruction(llvm::Instruction* I) const {

    using namespace llvm;

    IntrinsicsManager& im = IntrinsicsManager::getInstance();

    if (auto* gep = llvm::dyn_cast<GetElementPtrInst>(I)) {
        return util::view(gep->user_begin(), gep->user_end())
               .all_of([&im](const User* user) -> bool {
                   if (auto* call = dyn_cast<CallInst>(user)) {
                       if (function_type::UNKNOWN != im.getIntrinsicType(*call))
                           return true;
                   }
                   return false;
               });
    }

    return false;
}

CheckManager::~CheckManager() {}

char CheckManager::ID;
static RegisterPass<CheckManager>
X("check-manager", "Pass that manages other checker passes");

} /* namespace borealis */
