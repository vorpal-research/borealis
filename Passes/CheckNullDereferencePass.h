/*
 * CheckNullDereferencePass.h
 *
 *  Created on: Aug 24, 2012
 *      Author: ice-phoenix
 */

#ifndef CHECKNULLDEREFERENCEPASS_H_
#define CHECKNULLDEREFERENCEPASS_H_

#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Function.h>
#include <llvm/Instructions.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>

#include "DetectNullPass.h"
#include "PredicateStateAnalysis.h"
#include "SlotTrackerPass.h"

namespace borealis {

class CheckNullDereferencePass: public llvm::FunctionPass {

public:

	static char ID;

	CheckNullDereferencePass();
	virtual bool runOnFunction(llvm::Function& F);
	virtual void getAnalysisUsage(llvm::AnalysisUsage& Info) const;
	virtual ~CheckNullDereferencePass();

private:

	llvm::AliasAnalysis* AA;
	DetectNullPass* DNP;
	DetectNullPass::NullPtrSet* NullSet;
	PredicateStateAnalysis* PSA;
	SlotTracker* st;

	void processInst(const llvm::Instruction& I);
	void process(const llvm::LoadInst& I);
	void process(const llvm::StoreInst& I);

	void reportNullDereference(const llvm::Value& in, const llvm::Value& from);

	bool checkNullDereference(
	        const llvm::Instruction& where,
	        const llvm::Value& what);
};

} /* namespace borealis */

#endif /* CHECKNULLDEREFERENCEPASS_H_ */
