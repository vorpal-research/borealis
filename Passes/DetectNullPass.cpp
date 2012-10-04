/*
 * DetectNullPass.cpp
 *
 *  Created on: Aug 22, 2012
 *      Author: ice-phoenix
 */

#include "DetectNullPass.h"

#include <llvm/BasicBlock.h>
#include <llvm/Constants.h>

namespace borealis {

using util::contains;
using util::for_each;
using util::streams::endl;

DetectNullPass::DetectNullPass() : llvm::FunctionPass(ID) {
	// TODO
}

void DetectNullPass::getAnalysisUsage(llvm::AnalysisUsage& Info) const {
	using namespace::llvm;

	Info.setPreservesAll();
}

bool DetectNullPass::runOnFunction(llvm::Function& F) {
	using namespace::std;
	using namespace::llvm;

	nullInfoMap.clear();

	for_each(F, [this](const BasicBlock& BB){
		for_each(BB, [this](const Instruction& I){
			processInst(I);
		});
	});

	return false;
}

void DetectNullPass::processInst(const llvm::Instruction& I) {
	using namespace::llvm;

	if (containsKey(I)) return;

	if (isa<CallInst>(I))
		{ process(cast<CallInst>(I)); }

	else if (isa<StoreInst>(I))
		{ process(cast<StoreInst>(I)); }

	else if (isa<PHINode>(I))
		{ process(cast<PHINode>(I)); }

	else if (isa<InsertValueInst>(I))
		{ process(cast<InsertValueInst>(I)); }
}

void DetectNullPass::process(const llvm::CallInst& I) {
	if (!I.getType()->isPointerTy()) return;

	nullInfoMap[&I] = NullInfo().setStatus(Maybe_Null);
}

void DetectNullPass::process(const llvm::StoreInst& I) {
	using namespace::llvm;

	const Value* ptr = I.getPointerOperand();
	const Value* value = I.getValueOperand();

	if (!ptr->getType()->getPointerElementType()->isPointerTy()) return;

	if (isa<ConstantPointerNull>(*value)) {
		nullInfoMap[&I] = NullInfo().setStatus(Null);
	}
}

void DetectNullPass::process(const llvm::PHINode& I) {
	using namespace::llvm;

	if (!I.getType()->isPointerTy()) return;

	for (int i = 0; i < I.getNumIncomingValues(); i++) {
		const Value* incoming = I.getIncomingValue(i);
		if (isa<Instruction>(*incoming)) {
			processInst(cast<Instruction>(*incoming));
		} else {
			errs() << "Encountered non-instruction incoming value in PHINode: "
					<< *incoming
					<< endl;
		}
	}

	NullInfo nullInfo = NullInfo();
	for (int i = 0; i < I.getNumIncomingValues(); i++) {
		const Value* incoming = I.getIncomingValue(i);
		if (isa<Instruction>(*incoming)) {
			const Instruction& II = cast<Instruction>(*incoming);
			if (containsKey(II)) {
				nullInfo = nullInfo.merge(nullInfoMap[&II]);
			} else {
				nullInfo = nullInfo.merge(Not_Null);
			}
		}
	}
	nullInfoMap[&I] = nullInfo;
}

void DetectNullPass::process(const llvm::InsertValueInst& I) {
	using namespace::std;
	using namespace::llvm;

	const Value* value = I.getInsertedValueOperand();
	if (isa<ConstantPointerNull>(value)) {
		const vector<unsigned> idxs = I.getIndices().vec();
		nullInfoMap[&I] = NullInfo().setStatus(idxs, Null);
	}
}

bool DetectNullPass::containsKey(const llvm::Value& value) {
	return util::containsKey(nullInfoMap, &value);
}

DetectNullPass::~DetectNullPass() {
	// TODO
}

llvm::raw_ostream& operator <<(llvm::raw_ostream& s, const NullInfo& info) {
	using namespace::std;

	for_each(info.offsetInfoMap, [&s](const NullInfo::OffsetInfoMapEntry& entry){
		s << entry.first << "->" << entry.second << endl;
	});

	return s;
}

} /* namespace borealis */

char borealis::DetectNullPass::ID;
static llvm::RegisterPass<borealis::DetectNullPass>
X("detect-null", "Explicit NULL assignment detector", false, false);
