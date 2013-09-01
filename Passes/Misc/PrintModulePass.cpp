/*
 * PrintModulePass.cpp
 *
 *  Created on: Sep 6, 2012
 *      Author: belyaev
 */

#include <llvm/Pass.h>

#include "Logging/logger.hpp"
#include "Util/passes.hpp"

namespace borealis {

struct PrintModulePass :
        public llvm::ModulePass,
        public borealis::logging::ObjectLevelLogging<PrintModulePass> {

    static char ID;

    PrintModulePass() : llvm::ModulePass(ID), ObjectLevelLogging("module-printer") {}

    virtual bool runOnModule(llvm::Module& M) {
        infos() << endl << M << endl;
        return false;
    }

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const {
        AU.setPreservesAll();
    }

    virtual ~PrintModulePass() {}
};

char PrintModulePass::ID;
static RegisterPass<PrintModulePass>
X("module-print", "Pass that prints a module");

} // namespace borealis
