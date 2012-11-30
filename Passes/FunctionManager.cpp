/*
 * FunctionManager.cpp
 *
 *  Created on: Nov 15, 2012
 *      Author: ice-phoenix
 */

#include "FunctionManager.h"

#include "AnnotatorPass.h"
#include "Codegen/builtins.h"
#include "Codegen/intrinsics.h"
#include "Codegen/intrinsics_manager.h"
#include "Util/util.h"

namespace borealis {

using borealis::util::sayonara;

FunctionManager::FunctionManager() : llvm::ModulePass(ID) {}

void FunctionManager::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();
    AU.addRequiredTransitive<AnnotatorPass>();
}

void FunctionManager::addFunction(llvm::Function& F, PredicateState state) {

    if (data.count(&F) > 0) {
        sayonara(__FILE__, __LINE__, __PRETTY_FUNCTION__,
                "Attempt to register function " + F.getName().str() + " twice");
    }

    data[&F] = state;
}

PredicateState FunctionManager::get(
        llvm::Function& F,
        PredicateFactory* PF,
        TermFactory* TF) {

    intrinsic intr = IntrinsicsManager::getInstance().getIntrinsicType(F);
    if (intr != intrinsic::NOT_INTRINSIC) {
        return getPredicateState(intr, &F, PF, TF);
    }

    builtin bi = getBuiltInType(F);
    if (bi != builtin::NOT_BUILTIN) {
        return getPredicateState(bi, &F, PF, TF);
    }

    if (data.count(&F) == 0) {
        return sayonara<PredicateState>(__FILE__, __LINE__, __PRETTY_FUNCTION__,
                "Attempt to get unregistered function " + F.getName().str());
    }

    return data[&F];
}

char FunctionManager::ID;
static llvm::RegisterPass<FunctionManager>
X("function-manager", "Pass that manages function analysis results");

} /* namespace borealis */
