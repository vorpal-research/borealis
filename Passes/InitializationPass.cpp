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
    AUX<llvm::TargetData>::addRequiredTransitive(AU);
}

void InitializationPass::initializePass() {
    Z3ExprFactory::initialize(&GetAnalysis<llvm::TargetData>::doit(this));
}

char InitializationPass::ID;
static RegisterPass<InitializationPass>
X("init", "Initialization pass");

} /* namespace borealis */
