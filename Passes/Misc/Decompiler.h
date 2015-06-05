/*
 * Decompiler.h
 *
 *  Created on: 26 марта 2015 г.
 *      Author: kivi
 */

#ifndef DECOMPILER_H_
#define DECOMPILER_H_

#include <llvm/Pass.h>
#include <llvm/Analysis/LoopPass.h>
#include <llvm/Support/Debug.h>

#include "Passes/Misc/Decompiler/DecInstVisitor.h"
#include "Passes/Misc/Decompiler/BasicBlockInformation.h"
#include "Passes/Misc/Decompiler/PhiNodeInformation.h"
#include "Passes/Tracker/SlotTrackerPass.h"
#include "Util/passes.hpp"
#include "Logging/logger.hpp"

namespace borealis {
namespace decompiler{

class DecompilerPass : public llvm::ModulePass, public logging::ObjectLevelLogging<DecompilerPass> {
private:
    BasicBlockInformation bbInfo;
    PhiNodeInformation phiInfo;
    borealis::SlotTrackerPass* STP;

    void countBlocksInfo(llvm::Loop* L);

public:
    static char ID;
    DecompilerPass() : ModulePass(ID), ObjectLevelLogging("decompiler"), bbInfo(), phiInfo() {
    };

    virtual bool runOnModule(llvm::Module &M) override;
    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
    {
         AU.setPreservesAll();
         AU.addRequired<llvm::LoopInfo>();
         AUX<borealis::SlotTrackerPass>::addRequired(AU);
    }
};

} /* namespace decompiler */
} /* namespace borealis */

#endif /* DECOMPILER_H_ */
