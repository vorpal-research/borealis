/*
 * InitializationPass.cpp
 *
 *  Created on: Nov 15, 2012
 *      Author: ice-phoenix
 */

#include <llvm/Target/TargetData.h>

#include "Passes/InitializationPass.h"
#include "Solver/Z3ExprFactory.h"
#include "Util/passes.hpp"

namespace borealis {

void InitializationPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();
    AU.addRequiredTransitive<llvm::TargetData>();
}

void InitializationPass::initializePass() {
    Z3ExprFactory::initialize(&getAnalysis<llvm::TargetData>());
}

InitializationPass::~InitializationPass() {}

char InitializationPass::ID;
static RegisterPass<InitializationPass>
X("init", "Initialization pass");

} /* namespace borealis */
