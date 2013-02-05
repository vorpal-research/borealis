/*
 * FunctionManager.cpp
 *
 *  Created on: Nov 15, 2012
 *      Author: ice-phoenix
 */

#include "Codegen/intrinsics.h"
#include "Codegen/intrinsics_manager.h"
#include "Passes/AnnotatorPass.h"
#include "Passes/FunctionManager.h"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

FunctionManager::FunctionManager() : llvm::ModulePass(ID) {}

void FunctionManager::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();
    AU.addRequiredTransitive<AnnotatorPass>();
}

void FunctionManager::put(llvm::CallInst& CI, PredicateState state) {

    using borealis::util::containsKey;

    llvm::Function* F = CI.getCalledFunction();

    if (containsKey(data, F)) {
        BYE_BYE_VOID("Attempt to register function " + F->getName().str() + " twice");
    }

    data[F] = state;
}

PredicateState FunctionManager::get(
        llvm::CallInst& CI,
        PredicateFactory* PF,
        TermFactory* TF) {

    using borealis::util::containsKey;

    llvm::Function* F = CI.getCalledFunction();

    if (!containsKey(data, F)) {
        auto& m = IntrinsicsManager::getInstance();

        function_type ft = m.getIntrinsicType(CI);
        if (!isUnknown(ft)) {
            auto state = m.getPredicateState(ft, F, PF, TF);
            data[F] = state;
            return state;
        } else {
            return PredicateState();
        }
    } else {
        return data.at(F);
    }
}

char FunctionManager::ID;
static llvm::RegisterPass<FunctionManager>
X("function-manager", "Pass that manages function analysis results");

} /* namespace borealis */

#include "Util/unmacros.h"
