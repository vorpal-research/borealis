//
// Created by abdullin on 7/4/17.
//

#ifndef BOREALIS_ABSTRACTINTERPRETERPASS_H
#define BOREALIS_ABSTRACTINTERPRETERPASS_H

#include <llvm/Pass.h>

#include "Annotation/Annotation.h"
#include "Interpreter/Interpreter.h"

namespace borealis {

class LLVM_IR_Interpreter : public llvm::ModulePass {
public:

    static char ID;

    LLVM_IR_Interpreter() : llvm::ModulePass(ID) {}
    virtual ~LLVM_IR_Interpreter() = default;

    virtual bool runOnModule(llvm::Module& M) override;
    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;

};

}   /* namespace borealis */

#endif //BOREALIS_ABSTRACTINTERPRETERPASS_H
