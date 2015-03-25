/*
 * InstructionExecutor.h
 *
 *  Created on: Mar 19, 2015
 *      Author: belyaev
 */

#ifndef EXECUTOR_INSTRUCTIONEXECUTOR_H_
#define EXECUTOR_INSTRUCTIONEXECUTOR_H_

#include <unordered_map>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/IR/CallSite.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GetElementPtrTypeIterator.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/Support/DataTypes.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/raw_ostream.h>

#include "Executor/Arbiter.h"
#include "Codegen/intrinsics_manager.h"

namespace borealis {

class ExecutionEngine;
/*
 *
 */
class InstructionExecutor: public llvm::InstVisitor<InstructionExecutor> {
    ExecutionEngine* ee;
public:
    InstructionExecutor(ExecutionEngine* ee);
    ~InstructionExecutor();

    // Opcode Implementations
    void visitReturnInst(llvm::ReturnInst &I);
    void visitBranchInst(llvm::BranchInst &I);
    void visitSwitchInst(llvm::SwitchInst &I);
    void visitIndirectBrInst(llvm::IndirectBrInst &I);

    void visitBinaryOperator(llvm::BinaryOperator &I);
    void visitICmpInst(llvm::ICmpInst &I);
    void visitFCmpInst(llvm::FCmpInst &I);
    void visitAllocaInst(llvm::AllocaInst &I);
    void visitLoadInst(llvm::LoadInst &I);
    void visitStoreInst(llvm::StoreInst &I);
    void visitGetElementPtrInst(llvm::GetElementPtrInst &I);
    void visitPHINode(llvm::PHINode&) {
        throw std::logic_error("PHI nodes already handled!");
    }
    void visitTruncInst(llvm::TruncInst &I);
    void visitZExtInst(llvm::ZExtInst &I);
    void visitSExtInst(llvm::SExtInst &I);
    void visitFPTruncInst(llvm::FPTruncInst &I);
    void visitFPExtInst(llvm::FPExtInst &I);
    void visitUIToFPInst(llvm::UIToFPInst &I);
    void visitSIToFPInst(llvm::SIToFPInst &I);
    void visitFPToUIInst(llvm::FPToUIInst &I);
    void visitFPToSIInst(llvm::FPToSIInst &I);
    void visitPtrToIntInst(llvm::PtrToIntInst &I);
    void visitIntToPtrInst(llvm::IntToPtrInst &I);
    void visitBitCastInst(llvm::BitCastInst &I);
    void visitSelectInst(llvm::SelectInst &I);


    void visitCallSite(llvm::CallSite CS);
    void visitCallInst(llvm::CallInst &I) { visitCallSite (llvm::CallSite (&I)); }
    void visitInvokeInst(llvm::InvokeInst &I) { visitCallSite (llvm::CallSite (&I)); }
    void visitUnreachableInst(llvm::UnreachableInst &I);

    void visitShl(llvm::BinaryOperator &I);
    void visitLShr(llvm::BinaryOperator &I);
    void visitAShr(llvm::BinaryOperator &I);

    void visitVAArgInst(llvm::VAArgInst &I);
    void visitExtractElementInst(llvm::ExtractElementInst &I);
    void visitInsertElementInst(llvm::InsertElementInst &I);
    void visitShuffleVectorInst(llvm::ShuffleVectorInst &I);

    void visitExtractValueInst(llvm::ExtractValueInst &I);
    void visitInsertValueInst(llvm::InsertValueInst &I);

    void visitInstruction(llvm::Instruction &I) {
        llvm::errs() << I << "\n";
        throw std::logic_error("Instruction not interpretable yet!");
    }
};

} /* namespace borealis */

#endif /* EXECUTOR_INSTRUCTIONEXECUTOR_H_ */
