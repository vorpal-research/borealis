/*
 * Executor.h
 *
 *  Created on: Jan 27, 2015
 *      Author: belyaev
 */

#ifndef EXECUTOR_EXECUTIONENGINE_H_
#define EXECUTOR_EXECUTIONENGINE_H_

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
#include "Executor/MemorySimulator.h"
#include "Executor/InstructionExecutor.h"
#include "Executor/AnnotationExecutor.h"
#include "Codegen/intrinsics_manager.h"
#include "State/Transformer/AnnotationMaterializer.h"
#include "Passes/Tracker/SlotTrackerPass.h"

#include "Util/cache.hpp"

namespace borealis {

struct AllocaHolder{};

struct ExecutorContext {
    llvm::Function             *CurFunction;// The currently executing llvm::Function
    llvm::BasicBlock           *CurBB;      // The currently executing BB
    llvm::BasicBlock::iterator  CurInst;    // The next instruction to execute
    llvm::CallSite              Caller;     // Holds the call that called subframes.
    // NULL if main func or debugger invoked fn
    std::unordered_map<llvm::Value*, llvm::GenericValue> Values; // LLVM values used in this invocation
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


class ExecutionEngine {
    llvm::GenericValue ExitValue;          // The return llvm::Value of the Executor llvm::Function
    const llvm::DataLayout* TD;
    const llvm::TargetLibraryInfo* TLI;
    const SlotTrackerPass* ST;
    VariableInfoTracker* VIT;
    IntrinsicsManager* IM;
    Arbiter::Ptr Judicator;

    // The runtime stack of executing code.  The top of the stack is the current
    // llvm::Function record.
    std::vector<ExecutorContext> ECStack;

    // AtExitHandlers - List of functions to call when the program exits,
    // registered with the atexit() library llvm::Function.
    std::vector<llvm::Function*> AtExitHandlers;

    MemorySimulator Mem;

    util::cache<llvm::Function*, FactoryNest> FNCache;

    InstructionExecutor IE;

public:
    explicit ExecutionEngine(
        llvm::Module *M,
        const llvm::DataLayout* TD,
        const llvm::TargetLibraryInfo* TLI,
        const SlotTrackerPass* ST,
        VariableInfoTracker* VIT,
        Arbiter::Ptr Aldaris);
    ~ExecutionEngine();

    const llvm::DataLayout* getDataLayout() const { return TD; }
    ExecutorContext& getCurrentContext() { return ECStack.back(); }
    ExecutorContext& getContext(size_t which) { return ECStack.at(which); }
    bool isSymbolicPointer(void* ptr) {
        return Mem.isOpaquePointer(ptr);
    }

    void* getSymbolicPointer() {
        return Mem.getOpaquePtr();
    }

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
    void freeMachineCodeForFunction(llvm::Function*) { }

    // Methods used to execute code:
    // Place a call on the stack
    void executeCall(llvm::CallSite cs);
    void callFunction(llvm::Function *F, const std::vector<llvm::GenericValue> &ArgVals);
    void run(); // Execute instructions until nothing left to do

    util::option<llvm::GenericValue> callExternalFunction(
        llvm::Function *F,
        const std::vector<llvm::GenericValue> &ArgVals
    );
    void exitCalled(llvm::GenericValue GV);

    void addAtExitHandler(llvm::Function *F) {
        AtExitHandlers.push_back(F);
    }

    llvm::GenericValue *getFirstVarArg () {
        return &(ECStack.back ().VarArgs[0]);
    }

    util::option<llvm::GenericValue> callStdLibFunction(
        const llvm::Function* F,
        const std::vector<llvm::GenericValue>& ArgVals
    );

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

    void executeFAddInst(llvm::GenericValue &Dest, llvm::GenericValue Src1, llvm::GenericValue Src2, llvm::Type *Ty);
    void executeFSubInst(llvm::GenericValue &Dest, llvm::GenericValue Src1, llvm::GenericValue Src2, llvm::Type *Ty);
    void executeFMulInst(llvm::GenericValue &Dest, llvm::GenericValue Src1, llvm::GenericValue Src2, llvm::Type *Ty);
    void executeFDivInst(llvm::GenericValue &Dest, llvm::GenericValue Src1, llvm::GenericValue Src2, llvm::Type *Ty);
    void executeFRemInst(llvm::GenericValue &Dest, llvm::GenericValue Src1, llvm::GenericValue Src2, llvm::Type *Ty);
    llvm::GenericValue executeICMP_EQ(llvm::GenericValue Src1, llvm::GenericValue Src2, llvm::Type *Ty);
    llvm::GenericValue executeICMP_NE(llvm::GenericValue Src1, llvm::GenericValue Src2, llvm::Type *Ty);
    llvm::GenericValue executeICMP_ULT(llvm::GenericValue Src1, llvm::GenericValue Src2, llvm::Type *Ty);
    llvm::GenericValue executeICMP_SLT(llvm::GenericValue Src1, llvm::GenericValue Src2, llvm::Type *Ty);
    llvm::GenericValue executeICMP_UGT(llvm::GenericValue Src1, llvm::GenericValue Src2, llvm::Type *Ty);
    llvm::GenericValue executeICMP_SGT(llvm::GenericValue Src1, llvm::GenericValue Src2, llvm::Type *Ty);
    llvm::GenericValue executeICMP_ULE(llvm::GenericValue Src1, llvm::GenericValue Src2, llvm::Type *Ty);
    llvm::GenericValue executeICMP_SLE(llvm::GenericValue Src1, llvm::GenericValue Src2, llvm::Type *Ty);
    llvm::GenericValue executeICMP_UGE(llvm::GenericValue Src1, llvm::GenericValue Src2, llvm::Type *Ty);
    llvm::GenericValue executeICMP_SGE(llvm::GenericValue Src1, llvm::GenericValue Src2, llvm::Type *Ty);

    llvm::GenericValue executeFCMP_OEQ(llvm::GenericValue Src1, llvm::GenericValue Src2, llvm::Type *Ty);
    llvm::GenericValue executeFCMP_ONE(llvm::GenericValue Src1, llvm::GenericValue Src2, llvm::Type *Ty);
    llvm::GenericValue executeFCMP_OLE(llvm::GenericValue Src1, llvm::GenericValue Src2, llvm::Type *Ty);
    llvm::GenericValue executeFCMP_OGE(llvm::GenericValue Src1, llvm::GenericValue Src2, llvm::Type *Ty);
    llvm::GenericValue executeFCMP_OLT(llvm::GenericValue Src1, llvm::GenericValue Src2, llvm::Type *Ty);
    llvm::GenericValue executeFCMP_OGT(llvm::GenericValue Src1, llvm::GenericValue Src2, llvm::Type *Ty);
    llvm::GenericValue executeFCMP_UEQ(llvm::GenericValue Src1, llvm::GenericValue Src2, llvm::Type *Ty);
    llvm::GenericValue executeFCMP_UNE(llvm::GenericValue Src1, llvm::GenericValue Src2, llvm::Type *Ty);
    llvm::GenericValue executeFCMP_ULE(llvm::GenericValue Src1, llvm::GenericValue Src2, llvm::Type *Ty);
    llvm::GenericValue executeFCMP_UGE(llvm::GenericValue Src1, llvm::GenericValue Src2, llvm::Type *Ty);
    llvm::GenericValue executeFCMP_ULT(llvm::GenericValue Src1, llvm::GenericValue Src2, llvm::Type *Ty);
    llvm::GenericValue executeFCMP_UGT(llvm::GenericValue Src1, llvm::GenericValue Src2, llvm::Type *Ty);
    llvm::GenericValue executeFCMP_ORD(llvm::GenericValue Src1, llvm::GenericValue Src2, llvm::Type *Ty);
    llvm::GenericValue executeFCMP_UNO(llvm::GenericValue Src1, llvm::GenericValue Src2, llvm::Type *Ty);
    llvm::GenericValue executeFCMP_BOOL(llvm::GenericValue Src1, llvm::GenericValue Src2, const llvm::Type *Ty, const bool val);

    llvm::GenericValue executeCmpInst(unsigned predicate, llvm::GenericValue Src1, llvm::GenericValue Src2, llvm::Type *Ty);
    llvm::GenericValue executeSelectInst(llvm::GenericValue Src1, llvm::GenericValue Src2, llvm::GenericValue Src3, const llvm::Type *Ty);

    void popStackAndReturnValueToCaller(llvm::Type *RetTy, llvm::GenericValue Result);

    llvm::GenericValue executeAlloca(size_t size);

    llvm::GenericValue executeBinary(llvm::Instruction::BinaryOps opcode,
            llvm::GenericValue Src1,
            llvm::GenericValue Src2,
            llvm::Type* Ty);

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

#endif /* EXECUTOR_EXECUTIONENGINE_H_ */
