/*
 * DetectNullPass.cpp
 *
 *  Created on: Aug 22, 2012
 *      Author: ice-phoenix
 */

#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/BasicBlock.h>
#include <llvm/Constants.h>
#include <llvm/Instruction.h>
#include <llvm/Instructions.h>
#include <llvm/Support/raw_ostream.h>

#include <algorithm>

#include "DetectNullPass.h"



namespace borealis {
using util::contains;

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

	this->nullInfoMap.clear();

	for_each(F.begin(), F.end(), [this](const BasicBlock& BB){
		for_each(BB.begin(), BB.end(), [this](const Instruction& I){
			this->processInst(I);
		});
	});

	return false;
}

void DetectNullPass::processInst(const llvm::Instruction& I) {
	using namespace::llvm;

	if (this->containsInfoForValue(I)) return;

	if (isa<CallInst>(I)) {
		this->process(cast<CallInst>(I));

	} else if (isa<StoreInst>(I)) {
		this->process(cast<StoreInst>(I));

	} else if (isa<PHINode>(I)) {
		this->process(cast<PHINode>(I));

	} else if (isa<InsertValueInst>(I)) {
		this->process(cast<InsertValueInst>(I));
	}
}


void DetectNullPass::process(const llvm::CallInst& I) {
	if (!I.getType()->isPointerTy()) return;

	this->nullInfoMap[&I] = NullInfo().setStatus(Maybe_Null);
}

void DetectNullPass::process(const llvm::StoreInst& I) {
	using namespace::llvm;

	auto ptr = I.getPointerOperand();
	auto value = I.getValueOperand();

	if (!ptr->getType()->getPointerElementType()->isPointerTy()) return;

	if (isa<ConstantPointerNull>(value)) {
		this->nullInfoMap[&I] = NullInfo().setStatus(Null);
	}
}

void DetectNullPass::process(const llvm::PHINode& I) {
	using namespace::llvm;

	if (!I.getType()->isPointerTy()) return;

	for (unsigned i = 0; i < I.getNumIncomingValues(); i++) {
		auto incoming = I.getIncomingValue(i);
		if (isa<Instruction>(incoming)) {
			auto inst = cast<Instruction>(incoming);
			this->processInst(*inst);
		} else {
			errs() << "Encountered non-instruction incoming value in PHINode: "
					<< *incoming
					<< "\n";
		}
	}

	auto nullInfo = NullInfo();
	for (unsigned i = 0; i < I.getNumIncomingValues(); i++) {
		auto incoming = I.getIncomingValue(i);
		if (this->containsInfoForValue(*incoming)) {
			nullInfo = nullInfo.merge(this->nullInfoMap[incoming]);
		} else {
			nullInfo = nullInfo.merge(Not_Null);
		}
	}
	this->nullInfoMap[&I] = nullInfo;
}

void DetectNullPass::process(const llvm::InsertValueInst& I) {
	using namespace::llvm;

	auto value = I.getInsertedValueOperand();
	if (isa<ConstantPointerNull>(value)) {
		auto idxs = I.getIndices().vec();
		this->nullInfoMap[&I] = NullInfo().setStatus(idxs, Null);
	}
}

bool DetectNullPass::containsInfoForValue(const llvm::Value& value) {
	return contains(this->nullInfoMap, &value);
}

DetectNullPass::~DetectNullPass() {
	// TODO
}

llvm::raw_ostream& operator <<(llvm::raw_ostream& s, const NullInfo& info) {
	using namespace::std;

	for_each(info.offsetInfoMap.begin(), info.offsetInfoMap.end(), [&s](const NullInfo::OffsetInfoMapEntry& pair){
		s << pair.first << "->" << pair.second << "\n";
	});

	return s;
}

} /* namespace borealis */

char borealis::DetectNullPass::ID;
static llvm::RegisterPass<borealis::DetectNullPass>
X("detectnull", "Explicit NULL assignment detector", false, false);
