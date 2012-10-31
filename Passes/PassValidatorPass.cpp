/*
 * CheckNullDereferencePass.cpp
 *
 *  Created on: Aug 24, 2012
 *      Author: ice-phoenix
 */

#include "PassValidatorPass.h"

#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/BasicBlock.h>
#include <llvm/Constants.h>
#include <llvm/Instructions.h>

#include <type_traits>

#include "../lib/poolalloc/src/DSA/DataStructureAA.h"
#include "../util.h"

namespace borealis {

using namespace::llvm;

using util::for_each;
using util::streams::endl;

#define ITER_TYPE(M) const std::remove_reference<decltype(*M.begin())>::type&

PassValidatorPass::PassValidatorPass() : llvm::ModulePass(ID) {
	// TODO
}

void PassValidatorPass::getAnalysisUsage(llvm::AnalysisUsage& Info) const {
	using namespace::llvm;

	Info.setPreservesAll();
	Info.addRequiredTransitive<DSAA>();
}

static void printAARes(AliasAnalysis::AliasResult res) {
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
		for_each(M.getGlobalList(), c);
		for_each(M, [&](ITER_TYPE(M) F){
			if(!F.isDeclaration()) {
				for_each(F.getArgumentList(), c);
				for_each(F, [&](ITER_TYPE(F) BB){
					for_each(BB, c);
				});
			}
		});
	}
}

bool PassValidatorPass::runOnModule(llvm::Module& M) {
	auto paa = getAnalysisIfAvailable<DSAA>();
	auto ds = getAnalysisIfAvailable<TDDataStructures>();

	llvm::errs() << "Globals:" << endl;
	ds->getGlobalsGraph()->print(llvm::errs());
	llvm::errs() << endl;

	for_each(M, [&](ITER_TYPE(M) F){
		errs() << "Function " << F.getName() << ":" << endl;
		if(!F.isDeclaration()) ds->getDSGraph(F)->print(llvm::errs());
		else errs() << "External => No graph available!" << endl;
		errs() << endl;
	});

	if(!paa) errs() << "DSAA failed!" << endl;
	AliasAnalysis& aa = *paa;

	for_each_value(M, [&](const Value& v1){
		if (v1.getType()->isMetadataTy() || v1.getType()->isVoidTy())
			return;

		for_each_value(M, [&](const Value& v2){
			if (v2.getType()->isMetadataTy() || v2.getType()->isVoidTy())
				return;
			auto aliases = aa.alias(&v1,0,&v2,0);
			// only interesting cases
			if(&v1 != &v2 && (aliases == AliasAnalysis::MustAlias || aliases == AliasAnalysis::MayAlias)){
				errs() << v1 << endl;
				errs() << v2 << endl;
				printAARes(aliases);
				errs() << endl;
			}

		});
	});

	return false;
}

PassValidatorPass::~PassValidatorPass() {
	// TODO
}

} /* namespace borealis */

char borealis::PassValidatorPass::ID;
static llvm::RegisterPass<borealis::PassValidatorPass>
X("validator", "Dummy pass used for some validation");
