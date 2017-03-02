//
// Created by abdullin on 2/10/17.
//

#ifndef BOREALIS_INTERPRETER_H
#define BOREALIS_INTERPRETER_H

#include <deque>

#include <llvm/IR/InstVisitor.h>
#include <unordered_set>

#include "Interpreter/Environment.h"
#include "Interpreter/IR/Function.h"

namespace borealis {
namespace absint {

class Interpreter : public llvm::InstVisitor<Interpreter>, public logging::ObjectLevelLogging<Interpreter> {
public:

    Interpreter(const llvm::Module* module);

    void run();

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
    void visitExtractElementInst(llvm::ExtractElementInst& i);
    void visitInsertElementInst(llvm::InsertElementInst& i);
    void visitExtractValueInst(llvm::ExtractValueInst& i);
    void visitInsertValueInst(llvm::InsertValueInst& i);
    void visitBinaryOperator(llvm::BinaryOperator& i);
    void visitCallInst(llvm::CallInst& i);
    void visitBitCastInst(llvm::BitCastInst &i);

private:

    Domain::Ptr getVariable(const llvm::Value* value) const;

    Environment::Ptr environment_;
    const llvm::Module* module_;
    std::deque<Function::Ptr> callstack;
    std::unordered_map<std::string, int> fcount;
    State::Ptr currentState_;
};

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_INTERPRETER_H
