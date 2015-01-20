/*
 * InitializationPass.cpp
 *
 *  Created on: Nov 15, 2012
 *      Author: ice-phoenix
 */

#include <llvm/IR/DataLayout.h>

#include "Passes/Misc/InitializationPass.h"
#include "SMT/Z3/ExprFactory.h"
#include "Util/passes.hpp"

namespace borealis {

void InitializationPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();
    AUX<llvm::DataLayoutPass>::addRequiredTransitive(AU);
}

void InitializationPass::initializePass() {
    Z3::ExprFactory::initialize(&GetAnalysis<llvm::DataLayoutPass>::doit(this).getDataLayout());
}

char InitializationPass::ID;
static RegisterPass<InitializationPass>
X("init", "Initialization pass");

} /* namespace borealis */
