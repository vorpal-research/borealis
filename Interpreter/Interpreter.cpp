//
// Created by abdullin on 2/10/17.
//

#include <memory>

#include "Interpreter.h"

#include "Util/collections.hpp"
#include "Util/streams.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

Interpreter::Interpreter(const llvm::Module* module) : module_(module) {
    environment_ = Environment::Ptr{ new Environment(module_) };
    currentState_ = nullptr;
}

void Interpreter::run() {
    auto&& main = Function(environment_, module_->getFunction("main"));
    interpretFunction(&main, {});
}

void Interpreter::interpretFunction(Function* function, const std::vector<Domain::Ptr>& args) {
    // saving callstack
    auto oldState = currentState_;
    callstack.push(function);

    function->setArguments(args);

    for (auto&& bb : util::viewContainer(*function->getInstance())) {
        auto&& basicBlock = function->getBasicBlock(&bb);

        // updating output block with new information
        basicBlock->getOutputState()->merge(basicBlock->getInputState());
        currentState_ = basicBlock->getOutputState();
        visit(const_cast<llvm::BasicBlock&>(bb));

        // merging new information in successor blocks
        function->getOutputState()->merge(basicBlock->getOutputState());
        for (auto&& pred : function->getSuccessorsFor(&bb)) {
            pred->getInputState()->merge(basicBlock->getOutputState());
        }
    }
    errs() << endl << function << endl << endl;

    //restoring callstack
    callstack.pop();
    currentState_ = oldState;
}

/////////////////////////////////////////////////////////////////////
/// Visitors
/////////////////////////////////////////////////////////////////////
void Interpreter::visitInstruction(llvm::Instruction& i) {
    errs() << "Unsupported instruction: " << util::toString(i) << endl;
    return;
}

void Interpreter::visitReturnInst(llvm::ReturnInst& i) {
    auto&& retVal = i.getReturnValue();
    if (not retVal) return;

    auto&& retDomain = getVariable(retVal);
    if (retDomain) {
        currentState_->mergeToReturnValue(retDomain);
    }
}

void Interpreter::visitBranchInst(llvm::BranchInst&)             { /* ignore this */ }
void Interpreter::visitSwitchInst(llvm::SwitchInst&)             { /* ignore this */ }
void Interpreter::visitIndirectBrInst(llvm::IndirectBrInst&)     { /* ignore this */ }
void Interpreter::visitResumeInst(llvm::ResumeInst&)             { /* ignore this */ }
void Interpreter::visitUnreachableInst(llvm::UnreachableInst&)   { /* ignore this */ }

void Interpreter::visitICmpInst(llvm::ICmpInst& i) {
    auto&& lhv = getVariable(i.getOperand(0));
    auto&& rhv = getVariable(i.getOperand(1));
    if (not lhv || not rhv) return;

    auto&& result = lhv->icmp(rhv, i.getPredicate());
    currentState_->addLocalVariable(&i, result);
}

void Interpreter::visitFCmpInst(llvm::FCmpInst& i) {
    auto&& lhv = getVariable(i.getOperand(0));
    auto&& rhv = getVariable(i.getOperand(1));
    if (not lhv || not rhv) return;

    auto&& result = lhv->fcmp(rhv, i.getPredicate());
    currentState_->addLocalVariable(&i, result);
}

void Interpreter::visitAllocaInst(llvm::AllocaInst&) {
    // TODO: implement
}

void Interpreter::visitLoadInst(llvm::LoadInst&) {
    // TODO: implement
}

void Interpreter::visitStoreInst(llvm::StoreInst&) {
    // TODO: implement
}
void Interpreter::visitGetElementPtrInst(llvm::GetElementPtrInst&) {
    // TODO: implement
}

void Interpreter::visitPHINode(llvm::PHINode& i) {
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
    currentState_->addLocalVariable(&i, result);
}

void Interpreter::visitTruncInst(llvm::TruncInst& i) {
    auto&& value = getVariable(i.getOperand(0));
    if (not value) return;

    auto&& result = value->trunc(*i.getDestTy());
    currentState_->addLocalVariable(&i, result);
}

void Interpreter::visitZExtInst(llvm::ZExtInst& i) {
    auto&& value = getVariable(i.getOperand(0));
    if (not value) return;

    auto&& result = value->zext(*i.getDestTy());
    currentState_->addLocalVariable(&i, result);
}

void Interpreter::visitSExtInst(llvm::SExtInst& i) {
    auto&& value = getVariable(i.getOperand(0));
    if (not value) return;

    auto&& result = value->sext(*i.getDestTy());
    currentState_->addLocalVariable(&i, result);
}

void Interpreter::visitFPTruncInst(llvm::FPTruncInst& i) {
    auto&& value = getVariable(i.getOperand(0));
    if (not value) return;

    auto&& result = value->fptrunc(*i.getDestTy());
    currentState_->addLocalVariable(&i, result);
}

void Interpreter::visitFPExtInst(llvm::FPExtInst& i) {
    auto&& value = getVariable(i.getOperand(0));
    if (not value) return;

    auto&& result = value->fpext(*i.getDestTy());
    currentState_->addLocalVariable(&i, result);
}

void Interpreter::visitFPToUIInst(llvm::FPToUIInst& i) {
    auto&& value = getVariable(i.getOperand(0));
    if (not value) return;

    auto&& result = value->fptoui(*i.getDestTy());
    currentState_->addLocalVariable(&i, result);
}

void Interpreter::visitFPToSIInst(llvm::FPToSIInst& i) {
    auto&& value = getVariable(i.getOperand(0));
    if (not value) return;

    auto&& result = value->fptosi(*i.getDestTy());
    currentState_->addLocalVariable(&i, result);
}

void Interpreter::visitUIToFPInst(llvm::UIToFPInst& i) {
    auto&& value = getVariable(i.getOperand(0));
    if (not value) return;

    auto&& result = value->uitofp(*i.getDestTy());
    currentState_->addLocalVariable(&i, result);
}

void Interpreter::visitSIToFPInst(llvm::SIToFPInst& i) {
    auto&& value = getVariable(i.getOperand(0));
    if (not value) return;

    auto&& result = value->sitofp(*i.getDestTy());
    currentState_->addLocalVariable(&i, result);
}

void Interpreter::visitPtrToIntInst(llvm::PtrToIntInst& i) {
    auto&& value = getVariable(i.getOperand(0));
    if (not value) return;

    auto&& result = value->ptrtoint(*i.getDestTy());
    currentState_->addLocalVariable(&i, result);
}

void Interpreter::visitIntToPtrInst(llvm::IntToPtrInst& i) {
    auto&& value = getVariable(i.getOperand(0));
    if (not value) return;

    auto&& result = value->inttoptr(*i.getDestTy());
    currentState_->addLocalVariable(&i, result);
}

void Interpreter::visitSelectInst(llvm::SelectInst&) {
    // TODO: implement
}

void Interpreter::visitExtractElementInst(llvm::ExtractElementInst&) {
    // TODO: implement
}

void Interpreter::visitInsertElementInst(llvm::InsertElementInst&) {
    // TODO: implement
}

void Interpreter::visitExtractValueInst(llvm::ExtractValueInst&) {
    // TODO: implement
}

void Interpreter::visitInsertValueInst(llvm::InsertValueInst&) {
    // TODO: implement
}

void Interpreter::visitBinaryOperator(llvm::BinaryOperator& i) {
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

    currentState_->addLocalVariable(&i, result);
}

Domain::Ptr Interpreter::getVariable(const llvm::Value* value) const {
    if (auto&& result = currentState_->find(value)) {
        return result;

    } else if (auto&& constant = llvm::dyn_cast<llvm::Constant>(value)) {
        return environment_->getDomainFactory().get(constant);

    } else {
        return nullptr;
    }
}

void Interpreter::visitCallInst(llvm::CallInst& i) {
    if (i.getCalledFunction()->isDeclaration()) return;
    auto&& function = Function(environment_, i.getCalledFunction());

    std::vector<Domain::Ptr> args;
    for (auto j = 0U; j < i.getNumArgOperands(); ++j) {
        args.push_back(getVariable(i.getArgOperand(j)));
    }

    interpretFunction(&function, args);

    if (function.getOutputState()->getReturnValue()) {
        currentState_->addLocalVariable(&i, function.getOutputState()->getReturnValue());
    }
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"