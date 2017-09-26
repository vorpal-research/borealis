//
// Created by abdullin on 7/4/17.
//

#ifndef BOREALIS_ABSTRACTINTERPRETERPASS_H
#define BOREALIS_ABSTRACTINTERPRETERPASS_H

#include <llvm/Pass.h>

#include "Annotation/Annotation.h"
#include "Interpreter/Interpreter.h"

namespace borealis {

class AbstractInterpreterPass : public llvm::ModulePass {
public:

    static char ID;

    AbstractInterpreterPass() : llvm::ModulePass(ID) {};
    virtual ~AbstractInterpreterPass() = default;

    virtual bool runOnModule(llvm::Module& M) override;
    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;

};

}   /* namespace borealis */

#endif //BOREALIS_ABSTRACTINTERPRETERPASS_H
