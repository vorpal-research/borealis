/*
 * FunctionManager.cpp
 *
 *  Created on: Nov 15, 2012
 *      Author: ice-phoenix
 */

#include "FunctionManager.h"
#include "AnnotatorPass.h"
#include "Util/util.h"

namespace borealis {

using util::sayonara;

FunctionManager::FunctionManager() : llvm::ModulePass(ID) {}

void FunctionManager::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.addRequiredTransitive<AnnotatorPass>();
}

void FunctionManager::addFunction(llvm::Function& F, PredicateState state) {

    if (data.count(&F) > 0) {
        sayonara(__FILE__, __LINE__, __PRETTY_FUNCTION__,
                "Attempt to register function " + F.getName().str() + " twice");
    }

    data[&F] = state;
}

PredicateState FunctionManager::get(llvm::Function& F) {

    if (data.count(&F) == 0) {
        sayonara(__FILE__, __LINE__, __PRETTY_FUNCTION__,
                "Attempt to get unregistered function " + F.getName().str());
    }

    return data[&F];
}

char FunctionManager::ID;
static llvm::RegisterPass<FunctionManager>
X("function-manager", "Pass that manages function analysis results");

} /* namespace borealis */
