/*
 * InitializationPass.cpp
 *
 *  Created on: Nov 15, 2012
 *      Author: ice-phoenix
 */

#include <llvm/Target/TargetData.h>

#include "InitializationPass.h"
#include "Solver/Z3ExprFactory.h"

namespace borealis {

void InitializationPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.addRequiredTransitive<llvm::TargetData>();
}

void InitializationPass::initializePass() {
    Z3ExprFactory::initialize(&getAnalysis<llvm::TargetData>());
}

InitializationPass::~InitializationPass() {}

char InitializationPass::ID;
static llvm::RegisterPass<InitializationPass>
X("init", "Initialization pass");

} /* namespace borealis */
