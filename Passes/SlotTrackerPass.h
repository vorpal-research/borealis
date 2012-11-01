/*
 * SlotTrackerPass.h
 *
 *  Created on: Oct 4, 2012
 *      Author: belyaev
 */

#ifndef SLOTTRACKERPASS_H_
#define SLOTTRACKERPASS_H_

#include <llvm/Function.h>
#include <llvm/Module.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>

#include "Util/slottracker.h"
#include "Util/util.h"

using namespace::borealis::util::streams;
using std::unique_ptr;
using std::map;

using borealis::util::containsKey;

namespace borealis {

class SlotTrackerPass: public llvm::FunctionPass {

	typedef unique_ptr<borealis::SlotTracker> ptr_t;

	ptr_t globals;
	map<const llvm::Function*,ptr_t> funcs;

public:
	static char ID;

	SlotTrackerPass(): llvm::FunctionPass(ID) {}

	virtual bool doInitialization(llvm::Module& M);
	virtual bool runOnFunction(llvm::Function& F);
	virtual void getAnalysisUsage(llvm::AnalysisUsage& Info) const;

	SlotTracker* getSlotTracker (const llvm::Function* func) const;
	SlotTracker* getSlotTracker (const llvm::Module* _) const;
	SlotTracker* getSlotTracker (const llvm::BasicBlock* inst) const;
	SlotTracker* getSlotTracker (const llvm::Instruction* inst) const;
	SlotTracker* getSlotTracker (const llvm::Argument* arg) const;
	SlotTracker* getSlotTracker (const llvm::Function& func) const;
	SlotTracker* getSlotTracker (const llvm::Module& _) const;
	SlotTracker* getSlotTracker (const llvm::BasicBlock& bb) const;
	SlotTracker* getSlotTracker (const llvm::Instruction& inst) const;
	SlotTracker* getSlotTracker (const llvm::Argument& arg) const;

	virtual ~SlotTrackerPass() {}
};

}


#endif /* SLOTTRACKERPASS_H_ */
