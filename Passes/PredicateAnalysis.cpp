/*
 * PredicateAnalysis.cpp
 *
 *  Created on: Aug 31, 2012
 *      Author: ice-phoenix
 */

#include <llvm/ADT/StringMap.h>
#include <llvm/Assembly/Writer.h>
#include <llvm/BasicBlock.h>
#include <llvm/Constants.h>
#include <llvm/InstrTypes.h>
#include <llvm/Instruction.h>
#include <llvm/Instructions.h>
#include <llvm/Support/raw_ostream.h>

#include "PredicateAnalysis.h"

namespace borealis {

using util::for_each;
using util::toString;

std::string getValueName(const llvm::Value& v, const bool isSigned = false) {
	using namespace::llvm;

	if (isa<ConstantPointerNull>(v)) {
		return "<null>";
	} else if (isa<ConstantInt>(v)) {
		ConstantInt& cInt = cast<ConstantInt>(v);
		return isSigned
				? toString(cInt.getValue().getSExtValue())
	            : toString(cInt.getValue().getZExtValue());
	} else if (isa<ConstantFP>(v)) {
		ConstantFP& cFP = cast<ConstantFP>(v);
		return toString(cFP.getValueAPF().convertToDouble());
	}

	return v.hasName() ? v.getName().str() : toString(&v);
}



PredicateAnalysis::PredicateAnalysis() : llvm::FunctionPass(ID) {
	// TODO
}

void PredicateAnalysis::getAnalysisUsage(llvm::AnalysisUsage& Info) const {
	using namespace::llvm;

	Info.setPreservesAll();
}

bool PredicateAnalysis::runOnFunction(llvm::Function& F) {
	using namespace::std;
	using namespace::llvm;

	predicateMap.clear();

	for_each(F, [this](const BasicBlock& BB){
		for_each(BB, [this](const Instruction& I){
			processInst(I);
		});
	});

	return false;
}

void PredicateAnalysis::processInst(const llvm::Instruction& I) {
	using namespace::llvm;

	if (isa<LoadInst>(I))
		{ process(cast<LoadInst>(I)); }

	else if (isa<StoreInst>(I))
		{ process(cast<StoreInst>(I)); }

	else if (isa<BranchInst>(I))
		{ process(cast<BranchInst>(I)); }

	else if (isa<ICmpInst>(I))
		{ process(cast<ICmpInst>(I)); }
}

void PredicateAnalysis::process(const llvm::LoadInst& I) {
	using namespace::std;
	using namespace::llvm;

	const LoadInst* lhv = &I;
	const Value* rhv = I.getPointerOperand();

	predicateMap[&I] =
			getValueName(*lhv) + " = *" + getValueName(*rhv);
}

void PredicateAnalysis::process(const llvm::StoreInst& I) {
	using namespace::std;
	using namespace::llvm;

	const Value* lhv = I.getPointerOperand();
	const Value* rhv = I.getValueOperand();

	predicateMap[&I] =
			"*" + getValueName(*lhv) + " = " + getValueName(*rhv);
}

void PredicateAnalysis::process(const llvm::BranchInst& I) {
	using namespace::std;
	using namespace::llvm;

	if (I.isUnconditional()) return;

	Value* cond = I.getCondition();

	predicateMap[&I] =
			getValueName(*cond)  + " is true";
}

void PredicateAnalysis::process(const llvm::ICmpInst& I) {
	using namespace::std;
	using namespace::llvm;

	int pred = I.getPredicate();
	const Value* op1 = I.getOperand(0);
	const Value* op2 = I.getOperand(1);
	bool isSigned = I.isSigned();

	predicateMap[&I] =
			getValueName(*op1, isSigned) + " " + toString(pred) + " " + getValueName(*op2, isSigned);
}

PredicateAnalysis::~PredicateAnalysis() {
	// TODO
}

} /* namespace borealis */

char borealis::PredicateAnalysis::ID = 19;
static llvm::RegisterPass<borealis::PredicateAnalysis>
X("predicate", "Instruction predicate analysis", false, false);
