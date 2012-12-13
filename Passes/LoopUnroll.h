/*
 * LoopUnroll.h
 *
 *  Created on: Dec 7, 2012
 *      Author: ice-phoenix
 */

#ifndef LOOPUNROLL_H_
#define LOOPUNROLL_H_

#include <llvm/Analysis/LoopIterator.h>
#include <llvm/Analysis/LoopPass.h>
#include <llvm/Instructions.h>
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"

namespace borealis {

class LoopUnroll: public llvm::LoopPass {

public:

    static char ID;

    LoopUnroll();
    virtual void getAnalysisUsage(llvm::AnalysisUsage&) const;
    virtual bool runOnLoop(llvm::Loop*, llvm::LPPassManager&);

private:

    llvm::LoopInfo* LI;

};

} /* namespace borealis */

#endif /* LOOPUNROLL_H_ */
