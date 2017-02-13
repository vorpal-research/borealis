//
// Created by abdullin on 2/9/17.
//

#ifndef BOREALIS_IRVISITOR_H
#define BOREALIS_IRVISITOR_H

#include <llvm/IR/InstVisitor.h>
#include "Environment.h"
#include "State.h"

namespace borealis {
namespace absint {

class InstructionVisitor : public llvm::InstVisitor<InstructionVisitor> {
public:

    InstructionVisitor(const Environment* environment, State::Ptr state);

    const Environment& getEnvironment() const;
    State::Ptr getState() const;
    void setState(State::Ptr other);

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

private:

    Domain::Ptr getVariable(const llvm::Value* value) const;


    const Environment* environment_;
    State::Ptr state_;

};

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_IRVISITOR_H
