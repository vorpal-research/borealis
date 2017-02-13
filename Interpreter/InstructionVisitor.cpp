//
// Created by abdullin on 2/9/17.
//

#include <Util/streams.hpp>
#include "InstructionVisitor.h"
#include "Util/sayonara.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

InstructionVisitor::InstructionVisitor(const Environment* environment, State::Ptr state) : environment_(environment),
                                                                                           state_(state) {}

const Environment& InstructionVisitor::getEnvironment() const {
    return *environment_;
}

State::Ptr InstructionVisitor::getState() const {
    return state_;
}

void InstructionVisitor::setState(State::Ptr other) {
    state_ = other;
}

void InstructionVisitor::visitInstruction(llvm::Instruction& i) {
    errs() << "Unsupported instruction: " << util::toString(i) << endl;
    return;
}

void InstructionVisitor::visitReturnInst(llvm::ReturnInst& i) {
    auto&& retVal = i.getReturnValue();
    if (not retVal) return;

    auto&& retDomain = environment_->getDomainFactory().get(retVal);
    if (retDomain) {
        state_->mergeToReturnValue(retDomain);
    }
}

void InstructionVisitor::visitBranchInst(llvm::BranchInst&)             { /* ignore this */ }
void InstructionVisitor::visitSwitchInst(llvm::SwitchInst&)             { /* ignore this */ }
void InstructionVisitor::visitIndirectBrInst(llvm::IndirectBrInst&)     { /* ignore this */ }
void InstructionVisitor::visitResumeInst(llvm::ResumeInst&)             { /* ignore this */ }
void InstructionVisitor::visitUnreachableInst(llvm::UnreachableInst&)   { /* ignore this */ }

void InstructionVisitor::visitICmpInst(llvm::ICmpInst& i) {
    auto&& lhv = getVariable(i.getOperand(0));
    auto&& rhv = getVariable(i.getOperand(1));
    if (not lhv || not rhv) return;

    auto&& result = lhv->icmp(rhv, i.getPredicate());
    state_->addLocalVariable(&i, result);
}

void InstructionVisitor::visitFCmpInst(llvm::FCmpInst& i) {
    auto&& lhv = getVariable(i.getOperand(0));
    auto&& rhv = getVariable(i.getOperand(1));
    if (not lhv || not rhv) return;

    auto&& result = lhv->fcmp(rhv, i.getPredicate());
    state_->addLocalVariable(&i, result);
}

void InstructionVisitor::visitAllocaInst(llvm::AllocaInst&) {
    // TODO: implement
}

void InstructionVisitor::visitLoadInst(llvm::LoadInst&) {
    // TODO: implement
}

void InstructionVisitor::visitStoreInst(llvm::StoreInst&) {
    // TODO: implement
}
void InstructionVisitor::visitGetElementPtrInst(llvm::GetElementPtrInst&) {
    // TODO: implement
}

void InstructionVisitor::visitPHINode(llvm::PHINode& i) {
    auto&& result = getVariable(i.getIncomingValue(0));
    if (not result) return;

    for (auto j = 1U; j < i.getNumIncomingValues(); ++j) {
        auto&& incoming = getVariable(i.getIncomingValue(j));
        if (not incoming) {
            errs() << "No variable for " << util::toString(*i.getIncomingValue(j)) << " in " << util::toString(i) << endl;
            continue;
        }
        result = result->join(incoming);
    }
    state_->addLocalVariable(&i, result);
}

void InstructionVisitor::visitTruncInst(llvm::TruncInst& i) {
    auto&& value = getVariable(i.getOperand(0));
    if (not value) return;

    auto&& result = value->trunc(*i.getDestTy());
    state_->addLocalVariable(&i, result);
}

void InstructionVisitor::visitZExtInst(llvm::ZExtInst& i) {
    auto&& value = getVariable(i.getOperand(0));
    if (not value) return;

    auto&& result = value->zext(*i.getDestTy());
    state_->addLocalVariable(&i, result);
}

void InstructionVisitor::visitSExtInst(llvm::SExtInst& i) {
    auto&& value = getVariable(i.getOperand(0));
    if (not value) return;

    auto&& result = value->sext(*i.getDestTy());
    state_->addLocalVariable(&i, result);
}

void InstructionVisitor::visitFPTruncInst(llvm::FPTruncInst& i) {
    auto&& value = getVariable(i.getOperand(0));
    if (not value) return;

    auto&& result = value->fptrunc(*i.getDestTy());
    state_->addLocalVariable(&i, result);
}

void InstructionVisitor::visitFPExtInst(llvm::FPExtInst& i) {
    auto&& value = getVariable(i.getOperand(0));
    if (not value) return;

    auto&& result = value->fpext(*i.getDestTy());
    state_->addLocalVariable(&i, result);
}

void InstructionVisitor::visitFPToUIInst(llvm::FPToUIInst& i) {
    auto&& value = getVariable(i.getOperand(0));
    if (not value) return;

    auto&& result = value->fptoui(*i.getDestTy());
    state_->addLocalVariable(&i, result);
}

void InstructionVisitor::visitFPToSIInst(llvm::FPToSIInst& i) {
    auto&& value = getVariable(i.getOperand(0));
    if (not value) return;

    auto&& result = value->fptosi(*i.getDestTy());
    state_->addLocalVariable(&i, result);
}

void InstructionVisitor::visitUIToFPInst(llvm::UIToFPInst& i) {
    auto&& value = getVariable(i.getOperand(0));
    if (not value) return;

    auto&& result = value->uitofp(*i.getDestTy());
    state_->addLocalVariable(&i, result);
}

void InstructionVisitor::visitSIToFPInst(llvm::SIToFPInst& i) {
    auto&& value = getVariable(i.getOperand(0));
    if (not value) return;

    auto&& result = value->sitofp(*i.getDestTy());
    state_->addLocalVariable(&i, result);
}

void InstructionVisitor::visitPtrToIntInst(llvm::PtrToIntInst& i) {
    auto&& value = getVariable(i.getOperand(0));
    if (not value) return;

    auto&& result = value->ptrtoint(*i.getDestTy());
    state_->addLocalVariable(&i, result);
}

void InstructionVisitor::visitIntToPtrInst(llvm::IntToPtrInst& i) {
    auto&& value = getVariable(i.getOperand(0));
    if (not value) return;

    auto&& result = value->inttoptr(*i.getDestTy());
    state_->addLocalVariable(&i, result);
}

void InstructionVisitor::visitSelectInst(llvm::SelectInst&) {
    // TODO: implement
}

void InstructionVisitor::visitExtractElementInst(llvm::ExtractElementInst&) {
    // TODO: implement
}

void InstructionVisitor::visitInsertElementInst(llvm::InsertElementInst&) {
    // TODO: implement
}

void InstructionVisitor::visitExtractValueInst(llvm::ExtractValueInst&) {
    // TODO: implement
}

void InstructionVisitor::visitInsertValueInst(llvm::InsertValueInst&) {
    // TODO: implement
}

void InstructionVisitor::visitBinaryOperator(llvm::BinaryOperator& i) {
    auto&& lhv = getVariable(i.getOperand(0));
    auto&& rhv = getVariable(i.getOperand(1));
    if (not lhv || not rhv) return;

    Domain::Ptr result = nullptr;

    switch (i.getOpcode()) {
        case llvm::Instruction::Add:    result = lhv->add(rhv); break;
        case llvm::Instruction::FAdd:   result = lhv->fadd(rhv); break;
        case llvm::Instruction::Sub:    result = lhv->sub(rhv); break;
        case llvm::Instruction::FSub:   result = lhv->fsub(rhv); break;
        case llvm::Instruction::Mul:    result = lhv->mul(rhv); break;
        case llvm::Instruction::FMul:   result = lhv->fmul(rhv); break;
        case llvm::Instruction::UDiv:   result = lhv->udiv(rhv); break;
        case llvm::Instruction::SDiv:   result = lhv->sdiv(rhv); break;
        case llvm::Instruction::FDiv:   result = lhv->fdiv(rhv); break;
        case llvm::Instruction::URem:   result = lhv->urem(rhv); break;
        case llvm::Instruction::SRem:   result = lhv->srem(rhv); break;
        case llvm::Instruction::FRem:   result = lhv->frem(rhv); break;
        case llvm::Instruction::Shl:    result = lhv->shl(rhv); break;
        case llvm::Instruction::LShr:   result = lhv->lshr(rhv); break;
        case llvm::Instruction::AShr:   result = lhv->ashr(rhv); break;
        case llvm::Instruction::And:    result = lhv->bAnd(rhv); break;
        case llvm::Instruction::Or:     result = lhv->bOr(rhv); break;
        case llvm::Instruction::Xor:    result = lhv->bXor(rhv); break;
        default:
            UNREACHABLE("Unknown binary operator:" + i .getName().str());
    }

    state_->addLocalVariable(&i, result);
}

Domain::Ptr InstructionVisitor::getVariable(const llvm::Value* value) const {
    if (auto&& result = state_->find(value)) {
        return result;

    } else if (auto&& constant = llvm::dyn_cast<llvm::Constant>(value)) {
        return environment_->getDomainFactory().get(constant);

    } else {
        return nullptr;
    }
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"