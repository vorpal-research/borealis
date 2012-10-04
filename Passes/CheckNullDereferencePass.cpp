/*
 * CheckNullDereferencePass.cpp
 *
 *  Created on: Aug 24, 2012
 *      Author: ice-phoenix
 */

#include "CheckNullDereferencePass.h"

#include <llvm/BasicBlock.h>
#include <llvm/Constants.h>

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

	else if (isa<StoreInst>(I))
		{ process(cast<StoreInst>(I)); }
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

	errs() << "Possible NULL dereference in" << endl
			<< "\t" << in << endl
			<< "from" << endl
			<< "\t" << from << endl;
}

CheckNullDereferencePass::~CheckNullDereferencePass() {
	// TODO
}

} /* namespace borealis */

char borealis::CheckNullDereferencePass::ID;
static llvm::RegisterPass<borealis::CheckNullDereferencePass>
X("check-null-deref", "NULL dereference checker", false, false);
