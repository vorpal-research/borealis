/*
 * PrintModulePass.cpp
 *
 *  Created on: Sep 6, 2012
 *      Author: belyaev
 */


#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/BasicBlock.h>
#include <llvm/Constants.h>
#include <llvm/Instruction.h>
#include <llvm/Instructions.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Module.h>
#include <llvm/Function.h>
#include <llvm/BasicBlock.h>
#include <llvm/Instruction.h>
#include <llvm/Target/TargetData.h>

#include <llvm/Support/raw_ostream.h>

#include "../util.h"

using namespace llvm;
using namespace borealis::util::streams;

namespace {

struct PrintModulePass: public llvm::ModulePass {
	static char ID;

	PrintModulePass(): ModulePass(ID) {}
	virtual bool runOnModule(llvm::Module& F) {
		errs() << "Dumping module:" << endl
		       << F << endl;
		return false;
	}
	virtual void getAnalysisUsage(llvm::AnalysisUsage& Info) const {
		Info.setPreservesAll();
	}
	virtual ~PrintModulePass() {}
};


char PrintModulePass::ID;
static llvm::RegisterPass<PrintModulePass>
X("module-printer", "Pass that prints the module to standard output");

}

