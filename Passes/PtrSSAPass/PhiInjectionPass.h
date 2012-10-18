/*
 * PhiInjectionPass.h
 *
 *  Created on: Oct 18, 2012
 *      Author: belyaev
 */

#ifndef PHIINJECTIONPASS_H_
#define PHIINJECTIONPASS_H_

#include "llvm/Pass.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/CFG.h"
#include "llvm/Instructions.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include "llvm/Constants.h"
#include <deque>
#include <algorithm>
#include <unordered_set>
#include <tuple>

#include "../SlotTrackerPass.h"
#include "origin_tracker.h"

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

class PhiInjectionPass : public FunctionPass, public origin_tracker {
public:
    static char ID; // Pass identification, replacement for typeid.
    PhiInjectionPass() : FunctionPass(ID), DT_(nullptr), DF_(nullptr) {}
    void getAnalysisUsage(AnalysisUsage &AU) const;
    bool runOnFunction(Function&);

    SlotTracker& getSlotTracker(const Function*);

    Function* createNuevoFunc(Type* pointed, Module* daModule);

    void createNewDefs(BasicBlock &BB);
    void renameNewDefs(Instruction *newdef, Instruction* olddef, Value* ptr);

    typedef unordered_set<tuple<BasicBlock*, BasicBlock*, Value*>> phi_tracker;
    void propagateInstruction(Instruction& from, Instruction& to, phi_tracker&);

private:
    // Variables always live
    DominatorTree *DT_;
    DominanceFrontier *DF_;
};


} // namespace ptrssa
} // namespace borealis



#endif /* PHIINJECTIONPASS_H_ */
