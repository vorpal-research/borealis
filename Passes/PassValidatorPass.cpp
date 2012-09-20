/*
 * CheckNullDereferencePass.cpp
 *
 *  Created on: Aug 24, 2012
 *      Author: ice-phoenix
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


#include <set>
#include <unordered_set>

#include "../util.h"
#include "../lib/poolalloc/src/DSA/DataStructureAA.h"
#include "PassValidatorPass.h"

namespace borealis {

using llvm::DSAA;
using llvm::AliasAnalysis;
using util::streams::endl;
using llvm::Function;
using llvm::errs;
using borealis::util::for_each;

#define ITER_TYPE(M)  const std::remove_reference<decltype(*M.begin())>::type&

PassValidatorPass::PassValidatorPass() : llvm::ModulePass(ID) {
	// TODO
}

void PassValidatorPass::getAnalysisUsage(llvm::AnalysisUsage& Info) const {
	using namespace::llvm;

	Info.setPreservesAll();
	Info.addRequiredTransitive<DSAA>();
}

static void printARes(AliasAnalysis::AliasResult res) {
	switch(res) {
	case AliasAnalysis::NoAlias : errs() << "No alias"; return;
	case AliasAnalysis::MayAlias : errs() << "May alias"; return;
	case AliasAnalysis::MustAlias : errs() << "Must alias"; return;
	case AliasAnalysis::PartialAlias : errs() << "Partial alias"; return;
	default: errs() << "WTF?"; return;

	}
}
namespace {
	template<class Callable>
	void for_each_value(llvm::Module& M, Callable c) {
		for_each(M.getGlobalList(),c);
		for_each(M,[&](ITER_TYPE(M) F){
			for_each(F.getArgumentList(),c);
			for_each(F,[&](ITER_TYPE(F) BB) {
				for_each(BB,c);
			});
		});
	}
}

bool PassValidatorPass::runOnModule(llvm::Module& M) {
	using namespace llvm;

	auto ppt = getAnalysisIfAvailable<DSAA>();
	auto ds = getAnalysisIfAvailable<TDDataStructures>();

	errs() << "Globals:" << endl;
	ds->getGlobalsGraph()->print(errs());
	errs() << endl;

	for_each(M, [&](ITER_TYPE(M) F) {
		errs() << "Function " << F.getName() << ":" << endl;
		if(!F.isDeclaration()) ds->getDSGraph(F)->print(errs());
		else errs() << "External. No graph available" << endl;
		errs() << endl;
	});

	if(!ppt) errs () << "pointer failed" << "\n"; errs().flush();
	AliasAnalysis& gt = *ppt;

//	for_each_value(M,[&](const Value& v1){
//		for_each_value(M,[&](const Value& v2){
//
//			if(v1.getType()->isMetadataTy() ||
//			   v1.getType()->isVoidTy() ||
//			   v2.getType()->isMetadataTy() ||
//			   v2.getType()->isVoidTy()) return;
//
//			if(gt.alias(&v1,0,&v2,0) ==  AliasAnalysis::MustAlias) {
//				errs() << "Must alias relation:" << endl;
//				errs().indent(2);
//				errs() << v1 << endl;
//				errs().indent(2);
//				errs() << v2 << endl;
//			}
//		});
//	});

	return false;

}
PassValidatorPass::~PassValidatorPass() {
	// TODO
}

} /* namespace borealis */

char borealis::PassValidatorPass::ID;
static llvm::RegisterPass<borealis::PassValidatorPass>
X("validator", "Dummy pass used for validation");
