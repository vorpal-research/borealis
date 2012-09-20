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

#include "CheckNullDereferencePass.h"

namespace borealis {

using util::for_each;
using util::streams::endl;

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

	auto tmp = DNP->getNullSet();
	NullSet = &tmp;

	for_each(F, [this](const BasicBlock& BB){
		for_each(BB, [this](const Instruction& I){
			processInst(I);
		});
	});

	return false;
}

void CheckNullDereferencePass::processInst(const llvm::Instruction& I) {
	using namespace::llvm;

	if (isa<LoadInst>(I))
		{ process(cast<LoadInst>(I)); }
}

void CheckNullDereferencePass::process(const llvm::LoadInst& I) {
	using namespace::std;
	using namespace::llvm;

	const Value* ptr = I.getPointerOperand();

	for_each(*NullSet, [this, &I, ptr](const Value* nullValue){
		if (AA->alias(ptr, nullValue) != AliasAnalysis::AliasResult::NoAlias) {
			reportNullDereference(I, *nullValue);
		}
	});
}

void CheckNullDereferencePass::process(const llvm::StoreInst& I) {
	using namespace::std;
	using namespace::llvm;

	const Value* ptr = I.getPointerOperand();

	for_each(*NullSet, [this, &I, ptr](const Value* nullValue){
		if (AA->alias(ptr, nullValue) != AliasAnalysis::AliasResult::NoAlias) {
			reportNullDereference(I, *nullValue);
		}
	});
}

void CheckNullDereferencePass::reportNullDereference(
		const llvm::Value& in,
		const llvm::Value& from) {
	using namespace::llvm;

	errs() << "Possible NULL dereference in\n\t"
			<< in
			<< "\nfrom\n\t"
			<< from
			<< endl;
}

CheckNullDereferencePass::~CheckNullDereferencePass() {
	// TODO
}

} /* namespace borealis */

char borealis::CheckNullDereferencePass::ID = 18;
static llvm::RegisterPass<borealis::CheckNullDereferencePass>
X("check-null-deref", "NULL dereference checker", false, false);
