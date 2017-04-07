//
// Created by abdullin on 2/10/17.
//

#include "Interpreter.h"

#include "Util/collections.hpp"
#include "Util/streams.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

Interpreter::Interpreter(const llvm::Module* module) : ObjectLevelLogging("interpreter"), module_(module) {
    function_ = nullptr;
    state_ = nullptr;
}

void Interpreter::run() {
    auto&& main = module_.createFunction("main");
    if (main) {
        std::vector<Domain::Ptr> args;
        for (auto&& arg : main->getInstance()->getArgumentList()) {
            args.push_back(module_.getDomainFactory()->get(&arg));
        }

        interpretFunction(main, args);
        infos() << endl << module_ << endl;
    } else errs() << "No main function" << endl;
}

void Interpreter::interpretFunction(Function::Ptr function, const std::vector<Domain::Ptr>& args) {
    auto oldFunction = function_;
    auto oldState = state_;
    auto oldDeque = std::move(deque_);

    function_ = function;
    function_->setArguments(args);

    deque_.clear();
    deque_.push_back(&function->getInstance()->front());

    while (not deque_.empty()) {
        auto&& basicBlock = function_->getBasicBlock(deque_.front());

        // updating output block with new information
        basicBlock->getOutputState()->merge(basicBlock->getInputState());
        state_ = basicBlock->getOutputState();
        visit(const_cast<llvm::BasicBlock*>(deque_.front()));
        basicBlock->setVisited();

        // update function output block
        function_->getOutputState()->merge(state_);

        deque_.pop_front();
    }

    deque_ = std::move(oldDeque);
    state_ = oldState;
    function_ = oldFunction;
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
        state_->mergeToReturnValue(retDomain);
    }
}

void Interpreter::visitBranchInst(llvm::BranchInst& i) {
    std::vector<const llvm::BasicBlock*> successors;
    if (i.isConditional()) {
        auto&& cond = getVariable(i.getCondition());
        if (not cond || not cond->isValue()) {
            successors.push_back(i.getSuccessor(0));
            successors.push_back(i.getSuccessor(1));
        }

        auto&& boolean = llvm::dyn_cast<IntegerInterval>(cond.get());
        ASSERT(boolean && boolean->getWidth() == 1, "Non-bool in branch condition");

        successors.push_back( boolean->isConstant(1) ?
                              i.getSuccessor(0) :
                              i.getSuccessor(1) );
    } else {
        successors.push_back(i.getSuccessor(0));
    }

    addSuccessors(successors);
}

void Interpreter::visitSwitchInst(llvm::SwitchInst& i) {
    std::vector<const llvm::BasicBlock*> successors;
    auto&& cond = getVariable(i.getCondition());

    if (cond->isValue()) {
        bool isDefault = true;
        auto&& integer = llvm::dyn_cast<IntegerInterval>(cond.get());
        ASSERT(integer, "Non-integer condition in switch");

        for (auto&& cs : i.cases()) {
            if (integer->intersects(cs.getCaseValue()->getValue())) {
                successors.push_back(cs.getCaseSuccessor());
                isDefault = false;
            }
        }

        if (isDefault) {
            successors.push_back(i.getDefaultDest());
        }

    } else {
        for (auto j = 0U; j < i.getNumSuccessors(); ++j)
            successors.push_back(i.getSuccessor(j));
    }

    addSuccessors(successors);
}

void Interpreter::visitIndirectBrInst(llvm::IndirectBrInst& i) {
    std::vector<const llvm::BasicBlock*> successors;

    for (auto j = 0U; j < i.getNumSuccessors(); ++j) {
        successors.push_back(i.getSuccessor(j));
    }
    addSuccessors(successors);
}

void Interpreter::visitResumeInst(llvm::ResumeInst&)             { /* ignore this */ }
void Interpreter::visitUnreachableInst(llvm::UnreachableInst&)   { /* ignore this */ }

void Interpreter::visitICmpInst(llvm::ICmpInst& i) {
    auto&& lhv = getVariable(i.getOperand(0));
    auto&& rhv = getVariable(i.getOperand(1));
    if (not lhv || not rhv) return;

    auto&& result = lhv->icmp(rhv, i.getPredicate());
    state_->addVariable(&i, result);
}

void Interpreter::visitFCmpInst(llvm::FCmpInst& i) {
    auto&& lhv = getVariable(i.getOperand(0));
    auto&& rhv = getVariable(i.getOperand(1));
    if (not lhv || not rhv) return;

    auto&& result = lhv->fcmp(rhv, i.getPredicate());
    state_->addVariable(&i, result);
}

void Interpreter::visitAllocaInst(llvm::AllocaInst& i) {
    if (state_->find(&i)) {
        return;
    }
    auto&& ptr = module_.getDomainFactory()->getPointer(true);
    state_->addVariable(&i, ptr);
}

void Interpreter::visitLoadInst(llvm::LoadInst& i) {
    auto&& ptr = getVariable(i.getPointerOperand());
    if (not ptr) return;
    auto&& result = ptr->load(*i.getType(), {});
    if (result)
        state_->addVariable(&i, result);
}

void Interpreter::visitStoreInst(llvm::StoreInst&) {
    // TODO: implement
}

void Interpreter::visitGetElementPtrInst(llvm::GetElementPtrInst& i) {
    auto&& ptr = getVariable(i.getPointerOperand());
    if (not ptr) return;
    auto&& result = ptr->load(*i.getType(), {});
    state_->addVariable(&i, result);
}

void Interpreter::visitPHINode(llvm::PHINode& i) {
    Domain::Ptr result = nullptr;

    if ( (result = state_->find(&i)) ) {
        for (auto j = 0U; j < i.getNumIncomingValues(); ++j) {
            auto&& predecessor = function_->getBasicBlock(i.getIncomingBlock(j));
            auto&& incoming = getVariable(i.getIncomingValue(j));

            if (predecessor && predecessor->isVisited() && incoming) {
                result = result->widen(incoming);
            }
        }

    } else {
        // if not found, then result is nullptr
        for (auto j = 0U; j < i.getNumIncomingValues(); ++j) {
            auto&& predecessor = function_->getBasicBlock(i.getIncomingBlock(j));
            auto&& incoming = getVariable(i.getIncomingValue(j));

            if (predecessor && predecessor->isVisited() && incoming) {
                result = result ?
                         result->join(incoming) :
                         incoming;
            }
        }
    }

    if (result)
        state_->addVariable(&i, result);
}

#define CAST_INST(CAST) \
    auto&& value = getVariable(i.getOperand(0)); \
    if (not value) return; \
     \
    auto&& result = value->CAST(*i.getDestTy()); \
    if (result) state_->addVariable(&i, result); \
    else warns() << "Nullptr from " << #CAST << " operation" << endl; \


void Interpreter::visitTruncInst(llvm::TruncInst& i)        { CAST_INST(trunc);     }
void Interpreter::visitZExtInst(llvm::ZExtInst& i)          { CAST_INST(zext);      }
void Interpreter::visitSExtInst(llvm::SExtInst& i)          { CAST_INST(sext);      }
void Interpreter::visitFPTruncInst(llvm::FPTruncInst& i)    { CAST_INST(fptrunc);   }
void Interpreter::visitFPExtInst(llvm::FPExtInst& i)        { CAST_INST(fpext);     }
void Interpreter::visitFPToUIInst(llvm::FPToUIInst& i)      { CAST_INST(fptoui);    }
void Interpreter::visitFPToSIInst(llvm::FPToSIInst& i)      { CAST_INST(fptosi);    }
void Interpreter::visitUIToFPInst(llvm::UIToFPInst& i)      { CAST_INST(uitofp);    }
void Interpreter::visitSIToFPInst(llvm::SIToFPInst& i)      { CAST_INST(sitofp);    }
void Interpreter::visitPtrToIntInst(llvm::PtrToIntInst& i)  { CAST_INST(ptrtoint);  }
void Interpreter::visitIntToPtrInst(llvm::IntToPtrInst& i)  { CAST_INST(inttoptr);  }


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

    state_->addVariable(&i, result);
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
            auto&& result = module_.getDomainFactory()->getTop(*i.getType());
            if (result) state_->addVariable(&i, result);
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
        state_->addVariable(&i, function->getReturnValue());
    }
}

void Interpreter::visitBitCastInst(llvm::BitCastInst& i) {
    auto&& op = getVariable(i.getOperand(0));
    if (not op) return;

    auto&& result = op->bitcast(*i.getDestTy());
    if (result) state_->addVariable(&i, result);
}

///////////////////////////////////////////////////////////////
/// Util functions
///////////////////////////////////////////////////////////////
Domain::Ptr Interpreter::getVariable(const llvm::Value* value) {
    if (auto&& global = module_.findGLobal(value)) {
        return global;

    } else if (auto&& local = state_->find(value)) {
        return local;

    } else if (auto&& constant = llvm::dyn_cast<llvm::Constant>(value)) {
        return module_.getDomainFactory()->get(constant);

    } else if (value->getType()->isVoidTy()) {
        return nullptr;
    }
    return module_.getDomainFactory()->getTop(*value->getType());
}

void Interpreter::addSuccessors(const std::vector<const llvm::BasicBlock*>& successors) {
    for (auto&& it : successors) {

        auto&& successor = function_->getBasicBlock(it);
        successor->getInputState()->merge(state_);
        if (not successor->atFixpoint() && not util::contains(deque_, successor->getInstance())) {
            deque_.push_back(successor->getInstance());
        }

    }
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"