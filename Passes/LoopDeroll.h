/*
 * LoopDeroll.h
 *
 *  Created on: Dec 7, 2012
 *      Author: ice-phoenix
 */

#ifndef LOOPDEROLL_H_
#define LOOPDEROLL_H_

#include <llvm/Analysis/LoopPass.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Pass.h>

#include "Passes/LoopManager.h"

namespace borealis {

class LoopDeroll : public llvm::LoopPass {

public:

    static char ID;

    LoopDeroll();
    virtual void getAnalysisUsage(llvm::AnalysisUsage&) const override;
    virtual bool runOnLoop(llvm::Loop*, llvm::LPPassManager&) override;
    virtual ~LoopDeroll() {};

private:

    llvm::LoopInfo* LI;
    LoopManager* LM;
    llvm::ScalarEvolution* SE;

};

} /* namespace borealis */

#endif /* LOOPDEROLL_H_ */
