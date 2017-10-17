//
// Created by abdullin on 10/17/17.
//

#ifndef BOREALIS_FUNCTIONBITCASTOPTIMIZER_H
#define BOREALIS_FUNCTIONBITCASTOPTIMIZER_H

#include <unordered_set>

#include <llvm/IR/Instructions.h>
#include <llvm/Pass.h>

namespace borealis {

class FunctionBitCastKiller : public llvm::FunctionPass {

    using trash_set = std::unordered_set<llvm::Value*>;

public:
    static char ID;

    FunctionBitCastKiller(): llvm::FunctionPass(ID) {}
    virtual ~FunctionBitCastKiller() = default;

    void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    bool runOnFunction(llvm::Function& F) override;

private:

    void visitCallInst(llvm::CallInst& call);
    void copyMetadata(const llvm::Instruction &from, llvm::Instruction &to);
    llvm::Instruction* generateCastInst(llvm::Value* val, llvm::Type* to);

    trash_set deleted_instructions_;
};

}   // names[ace borealis


#endif //BOREALIS_FUNCTIONBITCASTOPTIMIZER_H
