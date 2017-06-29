//
// Created by abdullin on 2/10/17.
//

#include "Annotation/Annotation.h"
#include "Interpreter.h"

#include "Util/collections.hpp"

#include "Util/macros.h"
#include "ConditionSplitter.h"

namespace borealis {
namespace absint {

static config::BoolConfigEntry printModule("absint", "print-module");

Interpreter::Interpreter(const llvm::Module* module, FuncInfoProvider* FIP, SlotTrackerPass* st)
        : ObjectLevelLogging("interpreter"), module_(module, st), FIP_(FIP), ST_(st) {}

void Interpreter::run() {
    auto&& main = module_.create("main");
    if (main) {
        std::vector<Domain::Ptr> args;
        for (auto&& arg : main->getInstance()->getArgumentList()) {
            args.push_back(module_.getDomainFactory()->getTop(*arg.getType()));
        }

        interpretFunction(main, args);
        if (printModule.get(false)) infos() << endl << module_ << endl;
    } else {
        errs() << "No main function" << endl;
    }
}

const Module& Interpreter::getModule() const {
    return module_;
}

void Interpreter::interpretFunction(Function::Ptr function, const std::vector<Domain::Ptr>& args) {
    function->setArguments(args);
    stack_.push({ function, nullptr, {function->getEntryNode()}, {} });
    context_ = &stack_.top();

    while (not context_->deque.empty()) {
        auto&& basicBlock = context_->deque.front();

        // updating output block with new information
        auto&& inputBlock = basicBlock->getInputState();
        auto&& outputBlock = basicBlock->getOutputState();
        outputBlock->merge(inputBlock);
        context_->state = outputBlock;
        visit(const_cast<llvm::BasicBlock*>(basicBlock->getInstance()));
        basicBlock->setVisited();

        // update function output block
        context_->function->getOutputState()->merge(context_->state);

        context_->deque.pop_front();
    }
    // restore old context
    stack_.pop();
    context_ = &stack_.top();
}

Domain::Ptr Interpreter::getVariable(const llvm::Value* value) {
    if (auto&& global = module_.findGlobal(value)) {
        return global;

    } else if (auto&& local = context_->state->find(value)) {
        return local;

    } else if (auto&& constant = llvm::dyn_cast<llvm::Constant>(value)) {
        return module_.getDomainFactory()->get(constant);

    } else if (value->getType()->isVoidTy()) {
        return nullptr;
    }
    warns() << "Unknown value: " << ST_->toString(value) << endl;
    return nullptr;
}

/////////////////////////////////////////////////////////////////////
/// Visitors
/////////////////////////////////////////////////////////////////////
void Interpreter::visitInstruction(llvm::Instruction& i) {
    warns() << "Unsupported instruction: " << ST_->toString(&i) << endl;
    stub(i);
    return;
}

void Interpreter::visitReturnInst(llvm::ReturnInst& i) {
    if (not i.getReturnValue())
        return;

    auto&& retDomain = getVariable(i.getReturnValue());
    ASSERT(retDomain, "ret domain");
    context_->state->mergeToReturnValue(retDomain);
}

void Interpreter::visitBranchInst(llvm::BranchInst& i) {
    std::vector<BasicBlock*> successors;
    if (i.isConditional()) {
        auto cond = getVariable(i.getCondition());
        ASSERT(cond, "condition of branch");

        auto trueSuccessor = context_->function->getBasicBlock(i.getSuccessor(0));
        auto falseSuccessor = context_->function->getBasicBlock(i.getSuccessor(1));
        trueSuccessor->getInputState()->merge(context_->state);
        falseSuccessor->getInputState()->merge(context_->state);

        if (cond->isTop()) {
            auto splitted = ConditionSplitter(i.getCondition(), this, context_->state).apply();
            for (auto&& it : splitted) {
                trueSuccessor->getInputState()->addVariable(it.first, it.second.true_);
                falseSuccessor->getInputState()->addVariable(it.first, it.second.false_);
            }
            successors.push_back(trueSuccessor);
            successors.push_back(falseSuccessor);

        } else if (cond->isValue()) {
            auto boolean = llvm::dyn_cast<IntegerInterval>(cond.get());
            ASSERT(boolean && boolean->getWidth() == 1, "Non-bool in branch condition");

            auto successor = boolean->isConstant(1) ?
                             trueSuccessor :
                             falseSuccessor;
            successors.push_back(successor);
        } else {
            successors.push_back(trueSuccessor);
            successors.push_back(falseSuccessor);
        }
    } else {
        auto successor = context_->function->getBasicBlock(i.getSuccessor(0));
        successor->getInputState()->merge(context_->state);
        successors.push_back(successor);
    }

    addSuccessors(successors);
}

void Interpreter::visitSwitchInst(llvm::SwitchInst& i) {
    std::vector<BasicBlock*> successors;
    auto&& cond = getVariable(i.getCondition());

    if (cond->isValue()) {
        bool isDefault = true;
        auto&& integer = llvm::dyn_cast<IntegerInterval>(cond.get());
        ASSERT(integer, "Non-integer condition in switch");

        for (auto&& cs : i.cases()) {
            auto caseVal = module_.getDomainFactory()->toInteger(cs.getCaseValue()->getValue());
            if (integer->hasIntersection(caseVal)) {
                auto successor = context_->function->getBasicBlock(cs.getCaseSuccessor());
                successor->getInputState()->merge(context_->state);
                successors.push_back(successor);
                isDefault = false;
            }
        }

        if (isDefault) {
            auto successor = context_->function->getBasicBlock(i.getDefaultDest());
            successor->getInputState()->merge(context_->state);
            successors.push_back(successor);
        }

    } else {
        for (auto j = 0U; j < i.getNumSuccessors(); ++j) {
            auto successor = context_->function->getBasicBlock(i.getSuccessor(j));
            successor->getInputState()->merge(context_->state);
            successors.push_back(successor);
        }
    }

    addSuccessors(successors);
}

void Interpreter::visitIndirectBrInst(llvm::IndirectBrInst& i) {
    std::vector<BasicBlock*> successors;

    for (auto j = 0U; j < i.getNumSuccessors(); ++j) {
        auto successor = context_->function->getBasicBlock(i.getSuccessor(j));
        successor->getInputState()->merge(context_->state);
        successors.push_back(successor);
    }
    addSuccessors(successors);
}

void Interpreter::visitResumeInst(llvm::ResumeInst&)             { /* ignore this */ }
void Interpreter::visitUnreachableInst(llvm::UnreachableInst&)   { /* ignore this */ }

void Interpreter::visitICmpInst(llvm::ICmpInst& i) {
    auto&& lhv = getVariable(i.getOperand(0));
    auto&& rhv = getVariable(i.getOperand(1));
    ASSERT(lhv && rhv, "cmp args");

    auto&& result = lhv->icmp(rhv, i.getPredicate());
    ASSERT(result, "cmp result");
    context_->state->addVariable(&i, result);
}

void Interpreter::visitFCmpInst(llvm::FCmpInst& i) {
    auto&& lhv = getVariable(i.getOperand(0));
    auto&& rhv = getVariable(i.getOperand(1));
    ASSERT(lhv && rhv, "fcmp args");

    auto&& result = lhv->fcmp(rhv, i.getPredicate());
    ASSERT(result, "fcmp result");
    context_->state->addVariable(&i, result);
}

void Interpreter::visitAllocaInst(llvm::AllocaInst&) {
    UNREACHABLE("Alloca instructions replaced with borealis.alloca");
}

void Interpreter::visitLoadInst(llvm::LoadInst& i) {
    Domain::Ptr ptr;
    auto gepOper = llvm::dyn_cast<llvm::GEPOperator>(i.getPointerOperand());
    if (gepOper && not llvm::isa<llvm::GetElementPtrInst>(gepOper)) {
        ptr = gepOperator(*gepOper);
    } else {
        ptr = getVariable(i.getPointerOperand());
    }
    ASSERT(ptr, "load inst");

    auto&& result = ptr->load(*i.getType(), module_.getDomainFactory()->getIndex(0));
    ASSERT(result, "load result");
    context_->state->addVariable(&i, result);
}

void Interpreter::visitStoreInst(llvm::StoreInst& i) {
    auto&& ptr = getVariable(i.getPointerOperand());
    auto&& storeVal = getVariable(i.getValueOperand());
    ASSERT(ptr && storeVal, "store args");

    auto index = module_.getDomainFactory()->getIndex(0);
    if (context_->stores.find(&i) != context_->stores.end()) {
        auto load = ptr->load(*i.getValueOperand()->getType(), index);
        ptr->store(load->widen(storeVal), index);
    } else {
        ptr->store(storeVal, index);
        context_->stores.insert({&i, true});
    }
}

void Interpreter::visitGetElementPtrInst(llvm::GetElementPtrInst& i) {
    auto&& result = gepOperator(llvm::cast<llvm::GEPOperator>(i));
    ASSERT(result, "gep result");
    context_->state->addVariable(&i, result);
}

void Interpreter::visitPHINode(llvm::PHINode& i) {
    Domain::Ptr result = nullptr;

    if ( (result = context_->state->find(&i)) ) {
        for (auto j = 0U; j < i.getNumIncomingValues(); ++j) {
            auto&& predecessor = context_->function->getBasicBlock(i.getIncomingBlock(j));
            if (predecessor->isVisited()) {
                auto&& incoming = getVariable(i.getIncomingValue(j));
                ASSERT(incoming, "Unknown value in phi");
                result = result->widen(incoming);
            }
        }

    } else {
        // if not found, then result is nullptr
        for (auto j = 0U; j < i.getNumIncomingValues(); ++j) {
            auto&& predecessor = context_->function->getBasicBlock(i.getIncomingBlock(j));
            if (predecessor->isVisited()) {
                auto&& incoming = getVariable(i.getIncomingValue(j));
                ASSERT(incoming, "Unknown value in phi");

                result = result ?
                         result->join(incoming) :
                         incoming;
            }
        }
    }

    ASSERT(result, "phi result");
    context_->state->addVariable(&i, result);
}

#define CAST_INST(CAST) \
    auto&& value = getVariable(i.getOperand(0)); \
    ASSERT(value, "cast arg"); \
    auto&& result = value->CAST(*i.getDestTy()); \
    ASSERT(result, "cast result"); \
    context_->state->addVariable(&i, result);

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


void Interpreter::visitSelectInst(llvm::SelectInst& i) {
    auto cond = getVariable(i.getCondition());
    auto trueVal = getVariable(i.getTrueValue());
    auto falseVal = getVariable(i.getFalseValue());
    ASSERT(cond && trueVal && falseVal, "select args");

    Domain::Ptr result = cond->equals(trueVal.get()) ? trueVal :
                         cond->equals(falseVal.get()) ? falseVal :
                         module_.getDomainFactory()->getTop(*i.getType());
    context_->state->addVariable(&i, result);
}

void Interpreter::visitExtractValueInst(llvm::ExtractValueInst& i) {
    auto aggregate = getVariable(i.getAggregateOperand());
    ASSERT(aggregate, "extract value arg");

    std::vector<Domain::Ptr> indices;
    for (auto j = i.idx_begin(); j != i.idx_end(); ++j) {
        indices.push_back(module_.getDomainFactory()->getIndex(*j));
    }

    auto result = aggregate->extractValue(*i.getType(), indices);
    ASSERT(result, "extract value result");
    context_->state->addVariable(&i, result);
}

void Interpreter::visitInsertValueInst(llvm::InsertValueInst& i) {
    auto aggregate = getVariable(i.getAggregateOperand());
    auto value = getVariable(i.getInsertedValueOperand());
    ASSERT(aggregate && value, "insert value arg");

    std::vector<Domain::Ptr> indices;
    for (auto j = i.idx_begin(); j != i.idx_end(); ++j) {
        indices.push_back(module_.getDomainFactory()->getIndex(*j));
    }

    aggregate->insertValue(value, indices);
}

void Interpreter::visitBinaryOperator(llvm::BinaryOperator& i) {
    auto&& lhv = getVariable(i.getOperand(0));
    auto&& rhv = getVariable(i.getOperand(1));
    ASSERT(lhv && rhv, "binop args");

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
            UNREACHABLE("Unknown binary operator: " + i .getName().str());
    }

    ASSERT(result, "binop result");
    context_->state->addVariable(&i, result);
}

void Interpreter::visitCallInst(llvm::CallInst& i) {
    if (i.getCalledFunction() &&
            (i.getCalledFunction()->getName().startswith("borealis.alloc") ||
                    i.getCalledFunction()->getName().startswith("borealis.malloc"))) {
        handleMemoryAllocation(i);

    } else if (not i.getCalledFunction() || i.getCalledFunction()->isDeclaration()) {
        handleDeclaration(i);

    } else {
        std::vector<Domain::Ptr> args;
        Function::Ptr function = module_.get(i.getCalledFunction());
        if (function) {
            auto& oldArgs = function->getArguments();
            for (auto j = 0U; j < oldArgs.size(); ++j) {
                auto&& newArg = getVariable(i.getArgOperand(j));
                args.push_back(oldArgs[j]->widen(newArg));
            }

            if ( not util::equal(args, oldArgs,
                                 [](Domain::Ptr a, Domain::Ptr b) { return a->equals(b.get()); }) ) {
                interpretFunction(function, args);
            }

        } else {
            function = module_.create(i.getCalledFunction());
            for (auto j = 0U; j < i.getNumArgOperands(); ++j) {
                args.push_back(getVariable(i.getArgOperand(j)));
            }
            interpretFunction(function, args);
        }

        if (not i.getType()->isVoidTy()) {
            ASSERT(function->getReturnValue(), "extract value result");
            context_->state->addVariable(&i, function->getReturnValue());
        }
    }
}

void Interpreter::visitBitCastInst(llvm::BitCastInst& i) {
    auto&& op = getVariable(i.getOperand(0));
    ASSERT(op, "bitcast arg");

    auto&& result = op->bitcast(*i.getDestTy());
    ASSERT(result, "bitcast result");
    context_->state->addVariable(&i, result);
}

///////////////////////////////////////////////////////////////
/// Util functions
///////////////////////////////////////////////////////////////
void Interpreter::addSuccessors(const std::vector<BasicBlock*>& successors) {
    for (auto&& it : successors) {
        if (not it->atFixpoint() && not util::contains(context_->deque, it)) {
            context_->deque.push_back(it);
        }
    }
}

Domain::Ptr Interpreter::gepOperator(const llvm::GEPOperator& gep) {
    auto&& ptr = getVariable(gep.getPointerOperand());
    ASSERT(ptr, "gep args");

    std::vector<Domain::Ptr> offsets;
    for (auto j = gep.idx_begin(); j != gep.idx_end(); ++j) {
        auto val = llvm::cast<llvm::Value>(j);
        if (auto&& intConstant = llvm::dyn_cast<llvm::ConstantInt>(val)) {
            offsets.push_back(module_.getDomainFactory()->getIndex(*intConstant->getValue().getRawData()));
        } else if (auto indx = context_->state->find(val)) {
            offsets.push_back(indx);
        } else {
            UNREACHABLE("Non-integer constant in gep");
        }
    }

    return ptr->gep(*gep.getType()->getPointerElementType(), offsets);
}

void Interpreter::stub(const llvm::Instruction& i) {
    if (not i.getType()->isVoidTy()) {
        auto&& val = module_.getDomainFactory()->getTop(*i.getType());
        ASSERT(val, "stub result");
        context_->state->addVariable(&i, val);
    }
}

void Interpreter::handleMemoryAllocation(const llvm::CallInst& i) {
    auto&& size = getVariable(i.getArgOperand(2));
    if (not size) {
        warns() << "Allocating unknown amount of memory: " << ST_->toString(&i) << endl;
        stub(i);
        return;
    }

    auto&& integer = llvm::dyn_cast<IntegerInterval>(size.get());
    ASSERT(integer, "Non-integer domain in memory allocation");

    // Creating domain in memory
    // Adding new level of abstraction (pointer to array to real value), because:
    // - if this is alloc, we need one more level for correct GEP handler
    // - if this is malloc, we create array of dynamically allocated objects
    if (integer->ub()->isMax()) {
        stub(i);
        return;
    }

    auto&& arrayType = llvm::ArrayType::get(i.getType()->getPointerElementType(), integer->ub()->getRawValue());
    auto&& ptrType = llvm::PointerType::get(arrayType, 0);
    Domain::Ptr domain = module_.getDomainFactory()->allocate(*ptrType);

    ASSERT(domain, "malloc result");
    context_->state->addVariable(&i, domain);
}

void Interpreter::handleDeclaration(const llvm::CallInst& i) {
    if (not i.getCalledFunction() || i.getType()->isVoidTy()) {
        stub(i);
        return;
    }

    try {
        auto funcData = FIP_->getInfo(i.getCalledFunction());
        auto& argInfo = funcData.argInfo;
        for (auto j = 0U; j < i.getNumArgOperands(); ++j) {
            auto arg = i.getArgOperand(j);
            auto argType = arg->getType();
            if (argType->isPointerTy()) {
                if (argInfo[j].access == func_info::AccessPatternTag::Write || argInfo[j].access == func_info::AccessPatternTag::ReadWrite)
                    context_->state->addVariable(arg, module_.getDomainFactory()->getTop(*argType));
                else if (argInfo[j].access == func_info::AccessPatternTag::Delete)
                    context_->state->addVariable(arg, module_.getDomainFactory()->getBottom(*argType));
            }
        }

        if (not i.getType()->isVoidTy())
            context_->state->addVariable(&i, module_.getDomainFactory()->getTop(*i.getType()));

    } catch (std::out_of_range) {
        warns() << "Unknown function: " << ST_->toString(&i) << endl;
        // Unknown function possibly can do anything with pointer arguments
        // so we set all of them as TOP
        for (auto j = 0U; j < i.getNumArgOperands(); ++j) {
            auto arg = i.getArgOperand(j);
            auto argType = arg->getType();
            if (argType->isPointerTy()) {
                errs() << "Moving pointer to TOP: " << ST_->toString(arg) << endl;
                context_->state->addVariable(arg, module_.getDomainFactory()->getTop(*argType));
            }
        }
        if (not i.getType()->isVoidTy())
            context_->state->addVariable(&i, module_.getDomainFactory()->getTop(*i.getType()));

    }
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"