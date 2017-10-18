//
// Created by abdullin on 2/10/17.
//

#ifndef BOREALIS_INTERPRETER_H
#define BOREALIS_INTERPRETER_H

#include <memory>
#include <queue>

#include <llvm/IR/InstVisitor.h>
#include <Passes/Checker/CallGraphSlicer.h>

#include "Interpreter/IR/Function.h"
#include "Interpreter/IR/Module.h"
#include "Passes/Misc/FuncInfoProvider.h"
#include "Passes/Tracker/SlotTrackerPass.h"

namespace borealis {
namespace absint {

class Interpreter : public llvm::InstVisitor<Interpreter>, public logging::ObjectLevelLogging<Interpreter> {
public:

    Interpreter(const llvm::Module* module, FuncInfoProvider* FIP, SlotTrackerPass* st, CallGraphSlicer* cgs);

    void run();
    Module& getModule();

    void interpretFunction(Function::Ptr function, const std::vector<Domain::Ptr>& args);

    /// llvm instructions visitors
    void visitInstruction(llvm::Instruction& i);
    void visitReturnInst(llvm::ReturnInst& i);
    void visitBranchInst(llvm::BranchInst& i);
    void visitSwitchInst(llvm::SwitchInst& i);
    void visitIndirectBrInst(llvm::IndirectBrInst& i);
    void visitResumeInst(llvm::ResumeInst& i);
    void visitUnreachableInst(llvm::UnreachableInst& i);
    void visitICmpInst(llvm::ICmpInst& i);
    void visitFCmpInst(llvm::FCmpInst& i);
    void visitAllocaInst(llvm::AllocaInst& i);
    void visitLoadInst(llvm::LoadInst& i);
    void visitStoreInst(llvm::StoreInst& i);
    void visitGetElementPtrInst(llvm::GetElementPtrInst& i);
    void visitPHINode(llvm::PHINode& i);
    void visitTruncInst(llvm::TruncInst& i);
    void visitZExtInst(llvm::ZExtInst& i);
    void visitSExtInst(llvm::SExtInst& i);
    void visitFPTruncInst(llvm::FPTruncInst& i);
    void visitFPExtInst(llvm::FPExtInst& i);
    void visitFPToUIInst(llvm::FPToUIInst& i);
    void visitFPToSIInst(llvm::FPToSIInst& i);
    void visitUIToFPInst(llvm::UIToFPInst& i);
    void visitSIToFPInst(llvm::SIToFPInst& i);
    void visitPtrToIntInst(llvm::PtrToIntInst& i);
    void visitIntToPtrInst(llvm::IntToPtrInst& i);
    void visitSelectInst(llvm::SelectInst& i);
    void visitExtractValueInst(llvm::ExtractValueInst& i);
    void visitInsertValueInst(llvm::InsertValueInst& i);
    void visitBinaryOperator(llvm::BinaryOperator& i);
    void visitCallInst(llvm::CallInst& i);
    void visitBitCastInst(llvm::BitCastInst &i);

private:

    friend class ConditionSplitter;

    struct Context {
        Function::Ptr function; // current function
        State::Ptr state; // current state
        std::deque<BasicBlock*> deque; // deque of blocks to visit
        // This is generally fucked up
        std::unordered_set<const llvm::Value*> stores; // stores, visited in current context
    };

    /// Util functions
    Type::Ptr cast(const llvm::Type* type);
    Domain::Ptr gepOperator(const llvm::GEPOperator& gep);
    Domain::Ptr getVariable(const llvm::Value* value);
    void addSuccessors(const std::vector<BasicBlock*>& successors);
    Domain::Ptr handleFunctionCall(const llvm::Function* function,
                                   const std::vector<std::pair<const llvm::Value*, Domain::Ptr>>& args);
    Domain::Ptr handleMemoryAllocation(const llvm::Function* function,
                                       const std::vector<std::pair<const llvm::Value*, Domain::Ptr>>& args);
    Domain::Ptr handleDeclaration(const llvm::Function* function,
                                  const std::vector<std::pair<const llvm::Value*, Domain::Ptr>>& args);

    Module module_;
    TypeFactory::Ptr TF_;
    FuncInfoProvider* FIP_;
    SlotTrackerPass* ST_;
    CallGraphSlicer* CGS_;

    Context* context_;  // active context
    std::stack<Context> stack_; // stack of contexts of interpreter
};

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_INTERPRETER_H
