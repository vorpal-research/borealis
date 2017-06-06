//
// Created by abdullin on 2/10/17.
//

#include "Annotation/Annotation.h"
#include "Interpreter.h"

#include "Util/collections.hpp"
#include "Util/streams.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

static config::BoolConfigEntry printModule("absint", "print-module");

Interpreter::Interpreter(const llvm::Module* module, FuncInfoProvider* FIP)
        : ObjectLevelLogging("interpreter"), module_(module), FIP_(FIP) {
    function_ = nullptr;
    state_ = nullptr;
}

void Interpreter::run() {
    auto&& main = module_.createFunction("main");
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
    stub(i);
    return;
}

void Interpreter::visitReturnInst(llvm::ReturnInst& i) {
    auto&& retVal = i.getReturnValue();
    if (not retVal) {
        stub(i);
        return;
    }

    auto&& retDomain = getVariable(retVal);
    if (retDomain) {
        state_->mergeToReturnValue(retDomain);
    } else {
        stub(i);
    }
}

void Interpreter::visitBranchInst(llvm::BranchInst& i) {
    std::vector<const llvm::BasicBlock*> successors;
    if (i.isConditional()) {
        auto cond = getVariable(i.getCondition());
        if (not cond || not cond->isValue()) {
            successors.push_back(i.getSuccessor(0));
            successors.push_back(i.getSuccessor(1));

        } else {
            auto&& boolean = llvm::dyn_cast<IntegerInterval>(cond.get());
            ASSERT(boolean && boolean->getWidth() == 1, "Non-bool in branch condition");

            successors.push_back(boolean->isConstant(1) ?
                                 i.getSuccessor(0) :
                                 i.getSuccessor(1));
        }
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
            auto caseVal = module_.getDomainFactory()->getInt(cs.getCaseValue()->getValue());
            if (integer->intersects(caseVal)) {
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
    if (not lhv || not rhv) {
        stub(i);
        return;
    }

    auto&& result = lhv->icmp(rhv, i.getPredicate());
    if (result) state_->addVariable(&i, result);
    else stub(i);
}

void Interpreter::visitFCmpInst(llvm::FCmpInst& i) {
    auto&& lhv = getVariable(i.getOperand(0));
    auto&& rhv = getVariable(i.getOperand(1));
    if (not lhv || not rhv) {
        stub(i);
        return;
    }

    auto&& result = lhv->fcmp(rhv, i.getPredicate());
    if (result) state_->addVariable(&i, result);
    else stub(i);
}

void Interpreter::visitAllocaInst(llvm::AllocaInst& i) {
    if (state_->find(&i)) {
        return;
    }
    auto&& ptr = module_.getDomainFactory()->get(&i);
    if (ptr) state_->addVariable(&i, ptr);
    else stub(i);
}

void Interpreter::visitLoadInst(llvm::LoadInst& i) {
    Domain::Ptr ptr;
    auto gepOper = llvm::dyn_cast<llvm::GEPOperator>(i.getPointerOperand());
    if (gepOper && not llvm::isa<llvm::GetElementPtrInst>(gepOper)) {
        ptr = gepOperator(*gepOper);
    } else {
        ptr = getVariable(i.getPointerOperand());
    }
    if (not ptr) {
        stub(i);
        return;
    }

    auto&& result = ptr->load(*i.getType(), module_.getDomainFactory()->getIndex(0));
    if (result) state_->addVariable(&i, result);
    else stub(i);
}

void Interpreter::visitStoreInst(llvm::StoreInst& i) {
    auto&& ptr = getVariable(i.getPointerOperand());
    auto&& storeVal = getVariable(i.getValueOperand());
    if (not ptr || not storeVal) {
        stub(i);
        return;
    }
    if (stores_.find(&i) != stores_.end())
        ptr->store(module_.getDomainFactory()->getTop(*i.getValueOperand()->getType()), module_.getDomainFactory()->getIndex(0));
    else {
        ptr->store(storeVal, module_.getDomainFactory()->getIndex(0));
        stores_.insert({&i, true});
    }
}

void Interpreter::visitGetElementPtrInst(llvm::GetElementPtrInst& i) {
    auto&& result = gepOperator(llvm::cast<llvm::GEPOperator>(i));
    if (result) state_->addVariable(&i, result);
    else stub(i);
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

    if (result) state_->addVariable(&i, result);
    else stub(i);
}

#define CAST_INST(CAST) \
    auto&& value = getVariable(i.getOperand(0)); \
    if (not value) { \
        stub(i); \
        return; \
    }; \
    auto&& result = value->CAST(*i.getDestTy()); \
    if (result) state_->addVariable(&i, result); \
    else { \
        stub(i); \
        warns() << "Nullptr from " << #CAST << " operation" << endl; \
    }\

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
    if (not (cond && trueVal && falseVal)) {
        stub(i);
        return;
    }

    Domain::Ptr result = cond->equals(trueVal.get()) ? trueVal :
                         cond->equals(falseVal.get()) ? falseVal :
                         module_.getDomainFactory()->getTop(*i.getType());
    state_->addVariable(&i, result);
}

void Interpreter::visitExtractElementInst(llvm::ExtractElementInst&) {
    UNREACHABLE("ExtractElement Instruction is not implemented");
}

void Interpreter::visitInsertElementInst(llvm::InsertElementInst&) {
    UNREACHABLE("InsertElement Instruction is not implemented");
}

void Interpreter::visitExtractValueInst(llvm::ExtractValueInst& i) {
    auto aggregate = getVariable(i.getAggregateOperand());
    if (not aggregate) {
        stub(i);
        return;
    }

    std::vector<Domain::Ptr> indices;
    for (auto j = i.idx_begin(); j != i.idx_end(); ++j) {
        indices.push_back(module_.getDomainFactory()->getIndex(*j));
    }

    auto result = aggregate->extractValue(*i.getType(), indices);
    if (result) state_->addVariable(&i, result);
    else stub(i);
}

void Interpreter::visitInsertValueInst(llvm::InsertValueInst& i) {
    auto aggregate = getVariable(i.getAggregateOperand());
    auto value = getVariable(i.getInsertedValueOperand());
    if (not aggregate || not value) {
        return;
    }

    std::vector<Domain::Ptr> indices;
    for (auto j = i.idx_begin(); j != i.idx_end(); ++j) {
        indices.push_back(module_.getDomainFactory()->getIndex(*j));
    }

    aggregate->insertValue(value, indices);
}

void Interpreter::visitBinaryOperator(llvm::BinaryOperator& i) {
    auto&& lhv = getVariable(i.getOperand(0));
    auto&& rhv = getVariable(i.getOperand(1));
    if (not lhv || not rhv) {
        stub(i);
        return;
    }

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

    if (result) state_->addVariable(&i, result);
    else stub(i);
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
            }

        } else {
            function = module_.createFunction(i.getCalledFunction());
            for (auto j = 0U; j < i.getNumArgOperands(); ++j) {
                args.push_back(getVariable(i.getArgOperand(j)));
            }
            interpretFunction(function, args);
        }

        if (function->getReturnValue()) state_->addVariable(&i, function->getReturnValue());
        else stub(i);
    }
}

void Interpreter::visitBitCastInst(llvm::BitCastInst& i) {
    auto&& op = getVariable(i.getOperand(0));
    if (not op) {
        stub(i);
        return;
    }

    auto&& result = op->bitcast(*i.getDestTy());
    if (result) state_->addVariable(&i, result);
    else stub(i);
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
    errs() << "Unknown value: " << util::toString(*value) << endl;
    return nullptr;
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

Domain::Ptr Interpreter::gepOperator(const llvm::GEPOperator& gep) {
    auto&& ptr = getVariable(gep.getPointerOperand());
    if (not ptr) return nullptr;

    std::vector<Domain::Ptr> offsets;
    for (auto j = gep.idx_begin(); j != gep.idx_end(); ++j) {
        auto val = llvm::cast<llvm::Value>(j);
        if (auto indx = state_->find(val)) {
            offsets.push_back(indx);
        } else if (auto&& intConstant = llvm::dyn_cast<llvm::ConstantInt>(val)) {
            offsets.push_back(module_.getDomainFactory()->getIndex(*intConstant->getValue().getRawData()));
        } else {
            UNREACHABLE("Non-integer constant in gep");
        }
    }

    return ptr->gep(*gep.getType()->getPointerElementType(), offsets);
}

void Interpreter::stub(const llvm::Instruction& i) {
    if (not i.getType()->isVoidTy()) {
        auto&& val = module_.getDomainFactory()->getTop(*i.getType());
        if (val) state_->addVariable(&i, val);
    }
}

void Interpreter::handleMemoryAllocation(const llvm::CallInst& i) {
    auto&& size = getVariable(i.getArgOperand(2));
    if (not size) {
        errs() << "Allocating unknown amount of memory: " << util::toString(i) << endl;
        stub(i);
        return;
    }

    auto&& integer = llvm::dyn_cast<IntegerInterval>(size.get());
    ASSERT(integer, "Non-integer domain in memory allocation");

    // Creating domain in memory
    // Adding new level of abstraction (pointer to array to real value), because:
    // - if this is alloc, we need one more level for correct GEP handler
    // - if this is malloc, we create array of dynamically allocated objects
    if (integer->to()->isMax()) {
        stub(i);
        return;
    }

    auto&& arrayType = llvm::ArrayType::get(i.getType()->getPointerElementType(), integer->to()->getRawValue());
    auto&& ptrType = llvm::PointerType::get(arrayType, 0);
    Domain::Ptr domain = module_.getDomainFactory()->getInMemory(*ptrType);

    if (domain) state_->addVariable(&i, domain);
    else stub(i);
}

void Interpreter::handleDeclaration(const llvm::CallInst& i) {
    if (not i.getCalledFunction()) {
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
                    state_->addVariable(arg, module_.getDomainFactory()->getTop(*argType));
                else if (argInfo[j].access == func_info::AccessPatternTag::Delete)
                    state_->addVariable(arg, module_.getDomainFactory()->getBottom(*argType));
            }
        }

        if (not i.getType()->isVoidTy())
            state_->addVariable(&i, module_.getDomainFactory()->getTop(*i.getType()));

    } catch (std::out_of_range) {
        errs() << "Unknown function: " << util::toString(i) << endl;
        // Unknown function possibly can do anything with pointer arguments
        // so we set all of them as TOP
        for (auto j = 0U; j < i.getNumArgOperands(); ++j) {
            auto arg = i.getArgOperand(j);
            auto argType = arg->getType();
            if (argType->isPointerTy())
                state_->addVariable(arg, module_.getDomainFactory()->getTop(*argType));
        }
        if (not i.getType()->isVoidTy())
            state_->addVariable(&i, module_.getDomainFactory()->getTop(*i.getType()));

    }
}


}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"