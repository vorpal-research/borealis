/*
 * PhiInjectionPass.h
 *
 *  Created on: Oct 18, 2012
 *      Author: belyaev
 */

#ifndef PHIINJECTIONPASS_H_
#define PHIINJECTIONPASS_H_

#include <llvm/Analysis/Dominators.h>
#include <llvm/Constants.h>
#include <llvm/Instructions.h>
#include <llvm/Pass.h>
#include <llvm/Support/CFG.h>

#include <algorithm>
#include <tuple>
#include <unordered_set>

#include "Passes/ProxyFunctionPass.h"
#include "Passes/PtrSSAPass/origin_tracker.h"
#include "Passes/SlotTrackerPass.h"
#include "Util/passes.hpp"

namespace borealis {
namespace ptrssa {

class PhiInjectionPass :
    public ProxyFunctionPass,
    public ShouldBeModularized,
    public origin_tracker {

public:

    static char ID;

    typedef std::unordered_map<
        std::pair<llvm::BasicBlock*, llvm::Value*>,
        llvm::PHINode*
    > phi_tracker;

    PhiInjectionPass() : ProxyFunctionPass(ID) {}
    PhiInjectionPass(llvm::Pass* pass) : ProxyFunctionPass(ID, pass) {}

    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
    virtual bool runOnFunction(llvm::Function& F) override;
    virtual ~PhiInjectionPass() {}

    void propagateInstruction(llvm::Instruction& from, llvm::Instruction& to, phi_tracker& tracker);

private:

    llvm::DominatorTree* DT_;
};

} // namespace ptrssa
} // namespace borealis

#endif /* PHIINJECTIONPASS_H_ */
