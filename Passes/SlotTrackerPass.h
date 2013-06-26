/*
 * SlotTrackerPass.h
 *
 *  Created on: Oct 4, 2012
 *      Author: belyaev
 */

#ifndef SLOTTRACKERPASS_H_
#define SLOTTRACKERPASS_H_

#include <llvm/Pass.h>

#include "Util/slottracker.h"
#include "Util/util.h"

namespace borealis {

class SlotTrackerPass : public llvm::ModulePass {

	typedef std::unique_ptr<SlotTracker> ptr_t;

	ptr_t globals;
	std::map<const llvm::Function*, ptr_t> funcs;

public:

	static char ID;

	SlotTrackerPass() : llvm::ModulePass(ID) {}

	bool doInitialization(llvm::Module& M);
	virtual bool runOnModule(llvm::Module& M) override;
	virtual void getAnalysisUsage(llvm::AnalysisUsage& Info) const override;

	SlotTracker* getSlotTracker (const llvm::Function* func) const;
	SlotTracker* getSlotTracker (const llvm::Module* _) const;
	SlotTracker* getSlotTracker (const llvm::BasicBlock* bb) const;
	SlotTracker* getSlotTracker (const llvm::Instruction* inst) const;
	SlotTracker* getSlotTracker (const llvm::Argument* arg) const;
	SlotTracker* getSlotTracker (const llvm::Function& func) const;
	SlotTracker* getSlotTracker (const llvm::Module& _) const;
	SlotTracker* getSlotTracker (const llvm::BasicBlock& bb) const;
	SlotTracker* getSlotTracker (const llvm::Instruction& inst) const;
	SlotTracker* getSlotTracker (const llvm::Argument& arg) const;

	virtual ~SlotTrackerPass() {}
};

} // namespace borealis

#endif /* SLOTTRACKERPASS_H_ */
