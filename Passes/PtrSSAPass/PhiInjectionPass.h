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

namespace {
    using llvm::BasicBlockPass;
    using llvm::RegisterPass;
    using llvm::BasicBlock;
    using llvm::Function;
    using llvm::Module;
    using llvm::Value;
    using llvm::Constant;
    using llvm::Type;
    using llvm::Instruction;
    using llvm::isa;
    using llvm::CallInst;
    using llvm::AnalysisUsage;
    using llvm::dyn_cast;
    using namespace std;
}

class PhiInjectionPass :
    public ProxyFunctionPass<PhiInjectionPass>,
    public origin_tracker {
    typedef ProxyFunctionPass<PhiInjectionPass> base;
public:
    static char ID; // Pass identification, replacement for typeid.
    PhiInjectionPass() : base(), DT_(nullptr), DF_(nullptr) {}
    PhiInjectionPass(FunctionPass* del) : base(del), DT_(nullptr), DF_(nullptr) {}

    void getAnalysisUsage(AnalysisUsage &AU) const;
    bool runOnFunction(Function&);

    SlotTracker& getSlotTracker(const Function*);

    typedef unordered_map<pair<BasicBlock*, Value*>, PHINode*> phi_tracker;
    void propagateInstruction(Instruction& from, Instruction& to, phi_tracker&);

    virtual ~PhiInjectionPass(){}

private:
    // Variables always live
    DominatorTree *DT_;
    DominanceFrontier *DF_;
};


} // namespace ptrssa
} // namespace borealis



#endif /* PHIINJECTIONPASS_H_ */
