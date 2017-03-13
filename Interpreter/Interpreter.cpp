//
// Created by abdullin on 2/10/17.
//

#include <memory>
#include <queue>

#include "Interpreter.h"

#include "Util/collections.hpp"
#include "Util/streams.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

Interpreter::Interpreter(const llvm::Module* module) : ObjectLevelLogging("interpreter") {
    environment_ = Environment::Ptr{ new Environment(module) };
    module_ = Module(environment_);
    currentState_ = nullptr;
}

void Interpreter::run() {
    auto&& main = module_.createFunction("main");
    if (main) {
        std::vector<Domain::Ptr> args;
        for (auto&& arg : main->getInstance()->getArgumentList()) {
            args.push_back(environment_->getDomainFactory().get(&arg));
        }

        interpretFunction(main, args);
        infos() << module_ << endl;
    } else errs() << "No main function" << endl;
}

void Interpreter::interpretFunction(Function::Ptr function, const std::vector<Domain::Ptr>& args) {
    auto oldState = currentState_;

    function->setArguments(args);
    std::deque<const llvm::BasicBlock*> deque;
    deque.push_back(&function->getInstance()->front());

    while (not deque.empty()) {
        auto&& basicBlock = function->getBasicBlock(deque.front());

        // updating output block with new information
        basicBlock->getOutputState()->merge(basicBlock->getInputState());
        currentState_ = basicBlock->getOutputState();
        visit(const_cast<llvm::BasicBlock*>(deque.front()));

        // merging new information in successor blocks
        function->getOutputState()->merge(basicBlock->getOutputState());
        for (auto&& succ : function->getSuccessorsFor(basicBlock)) {
            succ->getInputState()->merge(basicBlock->getOutputState());
            if (not succ->atFixpoint() && not util::contains(deque, succ->getInstance())) {
                deque.push_back(succ->getInstance());
            }
        }
        deque.pop_front();
    }

    currentState_ = oldState;
}

/////////////////////////////////////////////////////////////////////
/// Visitors
/////////////////////////////////////////////////////////////////////
void Interpreter::visitInstruction(llvm::Instruction& i) {
    warns() << "Unsupported instruction: " << util::toString(i) << endl;
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

void Interpreter::visitAllocaInst(llvm::AllocaInst& i) {
    if (currentState_->find(&i)) {
        return;
    }
    auto&& ptr = environment_->getDomainFactory().getPointer(true);
    currentState_->addLocalVariable(&i, ptr);
}

void Interpreter::visitLoadInst(llvm::LoadInst& i) {
    auto&& ptr = getVariable(i.getPointerOperand());
    if (not ptr) return;
    auto&& result = ptr->load(*i.getType(), {});
    currentState_->addLocalVariable(&i, result);
}

void Interpreter::visitStoreInst(llvm::StoreInst&) {
    // TODO: implement
}
void Interpreter::visitGetElementPtrInst(llvm::GetElementPtrInst& i) {
    auto&& ptr = getVariable(i.getPointerOperand());
    if (not ptr) return;
    auto&& result = ptr->load(*i.getType(), {});
    currentState_->addLocalVariable(&i, result);
}

void Interpreter::visitPHINode(llvm::PHINode& i) {
    if (auto&& result = currentState_->find(&i)) {
        for (auto j = 0U; j < i.getNumIncomingValues(); ++j) {
            auto&& incoming = getVariable(i.getIncomingValue(j));
            if (not incoming) continue;

            result = result->widen(incoming);
        }
        currentState_->addLocalVariable(&i, result);
        return;
    }

    auto&& result = getVariable(i.getIncomingValue(0));
    if (not result) return;

    for (auto j = 1U; j < i.getNumIncomingValues(); ++j) {
        auto&& incoming = getVariable(i.getIncomingValue(j));
        if (not incoming) continue;

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

    } else if (value->getType()->isVoidTy()) {
        return nullptr;
    }
    return environment_->getDomainFactory().get(*value->getType(), Domain::TOP);
}

void Interpreter::visitCallInst(llvm::CallInst& i) {
    if (not i.getCalledFunction() || i.getCalledFunction()->isDeclaration()) return;

    std::vector<Domain::Ptr> args;

    Function::Ptr function = module_.contains(i.getCalledFunction());
    if (function) {
        auto& oldArgs = function->getArguments();
        for (auto j = 0U; j < oldArgs.size(); ++j) {
            auto&& newArg = getVariable(i.getArgOperand(j));
            args.push_back(oldArgs[j]->widen(newArg));
        }

        if (not util::equal(args, oldArgs, [](Domain::Ptr a, Domain::Ptr b) {
            return a->equals(b.get());
        })) {
            interpretFunction(function, args);
            auto&& result = environment_->getDomainFactory().get(&i, Domain::TOP);
            if (result) currentState_->addLocalVariable(&i, result);
            return;
        }

    } else {
        function = module_.createFunction(i.getCalledFunction());
        for (auto j = 0U; j < i.getNumArgOperands(); ++j) {
            args.push_back(getVariable(i.getArgOperand(j)));
        }
        interpretFunction(function, args);
    }

    if (function->getReturnValue()) {
        currentState_->addLocalVariable(&i, function->getReturnValue());
    }
}

void Interpreter::visitBitCastInst(llvm::BitCastInst& i) {
    auto&& op = getVariable(i.getOperand(0));
    if (not op) return;

    auto&& result = op->bitcast(*i.getDestTy());
    if (result) currentState_->addLocalVariable(&i, result);
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"