/*
 * PhiInjectionPass.h
 *
 *  Created on: Oct 18, 2012
 *      Author: belyaev
 */

#ifndef PHIINJECTIONPASS_H_
#define PHIINJECTIONPASS_H_

#include <llvm/Pass.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/Analysis/Dominators.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/Support/CFG.h>
#include <llvm/Instructions.h>
#include <llvm/Analysis/DominanceFrontier.h>
#include <llvm/Constants.h>
#include <deque>
#include <algorithm>
#include <unordered_set>
#include <tuple>

#include "Passes/SlotTrackerPass.h"
#include "origin_tracker.h"
#include "Passes/ProxyFunctionPass.hpp"

namespace borealis {
namespace ptrssa {

class PhiInjectionPass :
    public ProxyFunctionPass<PhiInjectionPass>,
    public origin_tracker {
    typedef ProxyFunctionPass<PhiInjectionPass> base;
public:
    static char ID; // Pass identification, replacement for typeid.
    PhiInjectionPass() : base(), DT_(nullptr), DF_(nullptr) {}
    PhiInjectionPass(FunctionPass* del) : base(del), DT_(nullptr), DF_(nullptr) {}

    void getAnalysisUsage(llvm::AnalysisUsage &AU) const;
    bool runOnFunction(llvm::Function&);

    SlotTracker& getSlotTracker(const llvm::Function*);

    typedef std::unordered_map<std::pair<llvm::BasicBlock*, llvm::Value*>, llvm::PHINode*> phi_tracker;
    void propagateInstruction(llvm::Instruction& from, llvm::Instruction& to, phi_tracker&);

    virtual ~PhiInjectionPass(){}

private:
    // Variables always live
    llvm::DominatorTree *DT_;
    llvm::DominanceFrontier *DF_;
};

} // namespace ptrssa
} // namespace borealis

#endif /* PHIINJECTIONPASS_H_ */
