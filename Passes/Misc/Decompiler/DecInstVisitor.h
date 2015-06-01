/*
 * Decompiler.h
 *
 *  Created on: 26 марта 2015 г.
 *      Author: kivi
 */

#ifndef DECINSTVISITOR_H_
#define DECINSTVISITOR_H_

#include <llvm/IR/InstVisitor.h>
#include <stack>
#include <unordered_set>
#include <Logging/logger.hpp>

#include "BasicBlockInformation.h"
#include "PhiNodeInformation.h"

namespace borealis {
namespace decompiler{

class DecInstVisitor : public llvm::InstVisitor<DecInstVisitor>, public logging::DelegateLogging {
private:
    int nesting;
    bool isOutGoto;
    bool isInIf;
    BBPosition currPos;
    std::stack<llvm::Instruction*> conditions;
    std::unordered_set<llvm::StructType*> usedStructs;
    BasicBlockInformation bbInfo;
    PhiNodeInformation phiInfo;

    void printType(llvm::Type* t);
    void printCond(llvm::Instruction* ins);
    void printTabulation(int num);
    void writeValueToStream(llvm::Value* v, bool deleteType = false);

    void printPhiInstructions(llvm::BasicBlock& bb);


    bool isString(llvm::Type* t);
    bool isStruct(llvm::Type* t);

public:
    DecInstVisitor() : nesting(1), isOutGoto(false), isInIf(false), currPos(BBPosition::NONE), conditions(), usedStructs(), bbInfo(), phiInfo() {};

    void setBasicBlockInfo(BasicBlockInformation& inf);
    void setPhiInfo(PhiNodeInformation& phi);

    void displayBasicBlock(llvm::BasicBlock& B,BBPosition position = BBPosition::NONE);
    void displayGlobals(llvm::Module& M);
    void displayFunction(llvm::Function& M);
    void displayStructs();

    void visitInstruction(llvm::Instruction &i);
    void visitStoreInst(llvm::StoreInst &i);
    void visitAllocaInst(llvm::AllocaInst &i);
    void visitLoadInst(llvm::LoadInst &i);
    void visitBinaryOperator(llvm::BinaryOperator &i);
    void visitICmpInst(llvm::ICmpInst &i);
    void visitFCmpInst(llvm::FCmpInst &i);
    void visitCastInst(llvm::CastInst& i);
    void visitCallInst(llvm::CallInst &i);
    void visitInvokeInst(llvm::InvokeInst& i);
    void visitLandingPadInst(llvm::LandingPadInst &i);
    void visitBranchInst(llvm::BranchInst &i);
    void visitSwitchInst(llvm::SwitchInst& i);
    void visitResumeInst(llvm::ResumeInst& i);
    void visitUnreachableInst(llvm::UnreachableInst& i);
    void visitPHINode(llvm::PHINode& phi);
    void visitSelectInst(llvm::SelectInst& i);
    void visitGetElementPtrInst(llvm::GetElementPtrInst& i);
    void visitExtractValueInst(llvm::ExtractValueInst& i);
    void visitInsertValueInst(llvm::InsertValueInst& i);
    void visitExtractElementInst(llvm::ExtractElementInst& i);
    void visitInsertElementInst(llvm::InsertElementInst& i);
    void visitReturnInst(llvm::ReturnInst &i);

};

} /* namespace decompiler */
} /* namespace borealis */

#endif /* DECINSTVISITOR_H_ */
