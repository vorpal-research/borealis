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

#include <set>
#include <unordered_set>

#include "CheckNullDereferencePass.h"

namespace borealis {

CheckNullDereferencePass::CheckNullDereferencePass() : llvm::FunctionPass(ID) {
	// TODO
}

void CheckNullDereferencePass::getAnalysisUsage(llvm::AnalysisUsage& Info) const {
	using namespace::llvm;

	Info.setPreservesAll();
	Info.addRequiredTransitive<AliasAnalysis>();
	Info.addRequiredTransitive<DetectNullPass>();
}

bool CheckNullDereferencePass::runOnFunction(llvm::Function& F) {
	using namespace::std;
	using namespace::llvm;

	AA = &getAnalysis<AliasAnalysis>();
	DNP = &getAnalysis<DetectNullPass>();

	auto ptr = DNP->getNullSet();
	NullSet = ptr.get();

	for_each(F.begin(), F.end(), [this](const BasicBlock& BB){
		for_each(BB.begin(), BB.end(), [this](const Instruction& I){
			this->processInst(I);
		});
	});

	NullSet = nullptr;

	return false;
}

void CheckNullDereferencePass::processInst(const llvm::Instruction& I) {
	using namespace::llvm;

	if (isa<LoadInst>(I)) {
		this->process(cast<LoadInst>(I));

	}
}

void CheckNullDereferencePass::process(const llvm::LoadInst& I) {
	using namespace::std;
	using namespace::llvm;

	auto ptr = I.getPointerOperand();

	for_each(NullSet->begin(), NullSet->end(), [this, &I, ptr](const Value* nullValue){
		if (AA->alias(ptr, nullValue) != AliasAnalysis::AliasResult::NoAlias) {
			errs() << "Possible NULL dereference in "
					<< I
					<< " from "
					<< *nullValue
					<< "\n";
		}
	});
}

CheckNullDereferencePass::~CheckNullDereferencePass() {
	// TODO
}

} /* namespace borealis */

char borealis::CheckNullDereferencePass::ID = 18;
static llvm::RegisterPass<borealis::CheckNullDereferencePass>
X("checknullderef", "NULL dereference checker", false, false);
