/*
 * Executor.h
 *
 *  Created on: Jan 27, 2015
 *      Author: belyaev
 */

#ifndef EXECUTOR_EXECUTOR_H_
#define EXECUTOR_EXECUTOR_H_

#include "Executor/MemorySimulator.h"

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

#include "Codegen/intrinsics_manager.h"

namespace borealis {

struct AllocaHolder{};

struct ExecutorContext {
    llvm::Function             *CurFunction;// The currently executing llvm::Function
    llvm::BasicBlock           *CurBB;      // The currently executing BB
    llvm::BasicBlock::iterator  CurInst;    // The next instruction to execute
    llvm::CallSite              Caller;     // Holds the call that called subframes.
    // NULL if main func or debugger invoked fn
    std::map<llvm::Value*, llvm::GenericValue> Values; // LLVM values used in this invocation
    std::vector<llvm::GenericValue> VarArgs; // Values passed through an ellipsis
    AllocaHolder Allocas;            // Track memory allocated by alloca

    ExecutorContext() : CurFunction(nullptr), CurBB(nullptr), CurInst(nullptr) {}

    ExecutorContext(ExecutorContext &&O)
    : CurFunction(O.CurFunction), CurBB(O.CurBB), CurInst(O.CurInst),
      Caller(O.Caller), Values(std::move(O.Values)),
      VarArgs(std::move(O.VarArgs)), Allocas(std::move(O.Allocas)) {}

    ExecutorContext &operator=(ExecutorContext &&O) {
        CurFunction = O.CurFunction;
        CurBB = O.CurBB;
        CurInst = O.CurInst;
        Caller = O.Caller;
        Values = std::move(O.Values);
        VarArgs = std::move(O.VarArgs);
        Allocas = std::move(O.Allocas);
        return *this;
    }
};


class Executor : public llvm::InstVisitor<Executor> {
    llvm::GenericValue ExitValue;          // The return llvm::Value of the Executor llvm::Function
    const llvm::DataLayout* TD;
    const llvm::TargetLibraryInfo* TLI;
    IntrinsicsManager* IM;

    // The runtime stack of executing code.  The top of the stack is the current
    // llvm::Function record.
    std::vector<ExecutorContext> ECStack;

    // AtExitHandlers - List of functions to call when the program exits,
    // registered with the atexit() library llvm::Function.
    std::vector<llvm::Function*> AtExitHandlers;

    MemorySimulator Mem;


public:
    explicit Executor(llvm::Module *M,  const llvm::DataLayout* TD, const llvm::TargetLibraryInfo* TLI);
    ~Executor();

    const llvm::DataLayout* getDataLayout() const { return TD; }

    /// runAtExitHandlers - Run any functions registered by the program's calls to
    /// atexit(3), which we intercept and store in AtExitHandlers.
    ///
    void runAtExitHandlers();


    /// run - Start execution with the specified llvm::Function and arguments.
    ///
    llvm::GenericValue runFunction(llvm::Function *F,
        const std::vector<llvm::GenericValue> &ArgValues);

    /// freeMachineCodeForFunction - The interpreter does not generate any code.
    ///
    void freeMachineCodeForFunction(llvm::Function *F) { }

    // Methods used to execute code:
    // Place a call on the stack
    void callFunction(llvm::Function *F, const std::vector<llvm::GenericValue> &ArgVals);
    void run();                // Execute instructions until nothing left to do

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
    void visitPHINode(llvm::PHINode &PN) {
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

    llvm::GenericValue callExternalFunction(llvm::Function *F,
        const std::vector<llvm::GenericValue> &ArgVals);
    void exitCalled(llvm::GenericValue GV);

    void addAtExitHandler(llvm::Function *F) {
        AtExitHandlers.push_back(F);
    }

    llvm::GenericValue *getFirstVarArg () {
        return &(ECStack.back ().VarArgs[0]);
    }

private:  // Helper functions
    llvm::GenericValue callStdLibFunction(const llvm::Function* F,
        const std::vector<llvm::GenericValue>& ArgVals);

    llvm::GenericValue executeGEPOperation(llvm::Value *Ptr, llvm::gep_type_iterator I,
        llvm::gep_type_iterator E, ExecutorContext &SF);

    // SwitchToNewBasicBlock - Start execution in a new basic block and run any
    // PHI nodes in the top of the block.  This is used for intraprocedural
    // control flow.
    //
    void SwitchToNewBasicBlock(llvm::BasicBlock *Dest, ExecutorContext &SF);

    void initializeExternalFunctions();
    llvm::GenericValue getConstantExprValue(llvm::ConstantExpr *CE, ExecutorContext &SF);
    llvm::GenericValue getOperandValue(llvm::Value *V, ExecutorContext &SF);
    llvm::GenericValue executeTruncInst(llvm::Value *SrcVal, llvm::Type *DstTy,
        ExecutorContext &SF);
    llvm::GenericValue executeSExtInst(llvm::Value *SrcVal, llvm::Type *DstTy,
        ExecutorContext &SF);
    llvm::GenericValue executeZExtInst(llvm::Value *SrcVal, llvm::Type *DstTy,
        ExecutorContext &SF);
    llvm::GenericValue executeFPTruncInst(llvm::Value *SrcVal, llvm::Type *DstTy,
        ExecutorContext &SF);
    llvm::GenericValue executeFPExtInst(llvm::Value *SrcVal, llvm::Type *DstTy,
        ExecutorContext &SF);
    llvm::GenericValue executeFPToUIInst(llvm::Value *SrcVal, llvm::Type *DstTy,
        ExecutorContext &SF);
    llvm::GenericValue executeFPToSIInst(llvm::Value *SrcVal, llvm::Type *DstTy,
        ExecutorContext &SF);
    llvm::GenericValue executeUIToFPInst(llvm::Value *SrcVal, llvm::Type *DstTy,
        ExecutorContext &SF);
    llvm::GenericValue executeSIToFPInst(llvm::Value *SrcVal, llvm::Type *DstTy,
        ExecutorContext &SF);
    llvm::GenericValue executePtrToIntInst(llvm::Value *SrcVal, llvm::Type *DstTy,
        ExecutorContext &SF);
    llvm::GenericValue executeIntToPtrInst(llvm::Value *SrcVal, llvm::Type *DstTy,
        ExecutorContext &SF);
    llvm::GenericValue executeBitCastInst(llvm::Value *SrcVal, llvm::Type *DstTy,
        ExecutorContext &SF);
    llvm::GenericValue executeCastOperation(llvm::Instruction::CastOps opcode, llvm::Value *SrcVal,
        llvm::Type *Ty, ExecutorContext &SF);
    void popStackAndReturnValueToCaller(llvm::Type *RetTy, llvm::GenericValue Result);

    llvm::GenericValue executeMalloc(const llvm::Function*, const std::vector<llvm::GenericValue> &ArgVals);
    llvm::GenericValue executeCalloc(const llvm::Function*, const std::vector<llvm::GenericValue> &ArgVals);
    llvm::GenericValue executeRealloc(const llvm::Function*, const std::vector<llvm::GenericValue> &ArgVals);
    llvm::GenericValue executeFree(const llvm::Function*, const std::vector<llvm::GenericValue> &ArgVals);
    llvm::GenericValue executeMemcpy(const llvm::Function*, const std::vector<llvm::GenericValue> &ArgVals);
    llvm::GenericValue executeMemset(const llvm::Function*, const std::vector<llvm::GenericValue> &ArgVals);
    llvm::GenericValue executeMemmove(const llvm::Function*, const std::vector<llvm::GenericValue> &ArgVals);

    using byte = uint8_t;
    void LoadValueFromMemory(llvm::GenericValue &Result, const byte* Ptr, llvm::Type *Ty);
    void StoreValueToMemory(const llvm::GenericValue &Val, byte* Ptr, llvm::Type *Ty);
};

} /* namespace borealis */

#endif /* EXECUTOR_EXECUTOR_H_ */
