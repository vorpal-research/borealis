/*
 * PrintModulePass.cpp
 *
 *  Created on: Sep 6, 2012
 *      Author: belyaev
 */

#include <llvm/Function.h>
#include <llvm/Module.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>

#include "../util.h"

using namespace::borealis::util::streams;

namespace borealis {

struct PrintModulePass: public llvm::ModulePass {

	static char ID;

	PrintModulePass(): llvm::ModulePass(ID) {}

	virtual bool runOnModule(llvm::Module& M) {
		using namespace::llvm;
		errs() << "Dumping module:" << endl
		       << M << endl;
		return false;
	}

	virtual void getAnalysisUsage(llvm::AnalysisUsage& Info) const {
		Info.setPreservesAll();
	}

	virtual ~PrintModulePass() {}
};

char PrintModulePass::ID;
static llvm::RegisterPass<PrintModulePass>
X("module-printer", "Pass that prints a module to cout");

}
