//
// Created by abdullin on 2/10/17.
//

#include "Annotation/Annotation.h"
#include "Config/config.h"
#include "IRInterpreter.h"
#include "Interpreter/Domain/ConditionSplitter.h"
#include "Util/collections.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

static config::BoolConfigEntry printModule("absint", "print-module");

IRInterpreter::IRInterpreter(const llvm::Module* module, FuncInfoProvider* FIP, SlotTrackerPass* st, CallGraphSlicer* cgs)
        : ObjectLevelLogging("ir-interpreter"), module_(module, st), TF_(TypeFactory::get()), FIP_(FIP), ST_(st), CGS_(cgs) {}

void IRInterpreter::run() {
    auto&& roots = module_.getRootFunctions();
    if (roots.empty()) {
        errs() << "No root function" << endl;
        return;
    }

    for (auto&& root : roots) {
        std::vector<Domain::Ptr> args;
        for (auto&& arg : root->getInstance()->getArgumentList()) {
            args.emplace_back(module_.getDomainFactory()->getTop(cast(arg.getType())));
        }

        interpretFunction(root, args);
    }

    for (auto&& function : CGS_->getAddressTakenFunctions()) {
        auto&& ai_function = module_.get(function);
        if (not function->isDeclaration() && not ai_function->isVisited()) {
            std::vector<Domain::Ptr> topargs;
            for (auto&& arg : function->args())
                topargs.emplace_back(module_.getDomainFactory()->getTop(cast(arg.getType())));
            interpretFunction(ai_function, topargs);
        }
    }

    if (printModule.get(false)) {
        infos() << endl << module_ << endl;
    }
    ASSERT(stack_.empty(), "Stack is not empty after interpretation");
    ASSERT(not context_, "Context is not nullptr");
}

Module& IRInterpreter::getModule() {
    return module_;
}

void IRInterpreter::interpretFunction(Function::Ptr function, const std::vector<Domain::Ptr>& args) {
    // if arguments and globals are not changed, then this function don't need to be reinterpreted
    // if function's argument list is empty, then check if it's visited at all
    auto updArgs = function->getArguments().empty() ?
                   (not function->getEntryNode()->isVisited()) :
                   function->updateArguments(args);
    auto updGlobals = function->updateGlobals(module_.getGlobalsFor(function));
    if (not (updArgs || updGlobals)) return;
    stack_.push({ function, nullptr, {function->getEntryNode()}, {} });
    context_ = &stack_.top();

    while (not context_->deque.empty()) {
        auto&& basicBlock = context_->deque.front();
        basicBlock->updateGlobals(module_.getGlobalsFor(basicBlock));

        // updating output block with new information
        basicBlock->mergeOutputWithInput();
        context_->state = basicBlock->getOutputState();
        visit(const_cast<llvm::BasicBlock*>(basicBlock->getInstance()));
        basicBlock->setVisited();

        // update function output block
        context_->function->mergeToOutput(context_->state);

        context_->deque.pop_front();
    }
    // restore old context
    stack_.pop();
    context_ = stack_.empty() ?
               nullptr :
               &stack_.top();
}

Domain::Ptr IRInterpreter::getVariable(const llvm::Value* value) {
    if (auto&& global = module_.findGlobal(value)) {
        return global;

    } else if (auto&& local = context_->state->find(value)) {
        return local;

    } else if (auto&& constant = llvm::dyn_cast<llvm::Constant>(value)) {
        return module_.getDomainFactory()->get(constant);

    } else if (value->getType()->isVoidTy() || value->getType()->isMetadataTy()) {
        return nullptr;
    }
    warns() << "Unknown value: " << ST_->toString(value) << endl;
    return nullptr;
}

/////////////////////////////////////////////////////////////////////
/// Visitors
/////////////////////////////////////////////////////////////////////
void IRInterpreter::visitInstruction(llvm::Instruction& i) {
    warns() << "Unsupported instruction: " << ST_->toString(&i) << endl;
    if (not i.getType()->isVoidTy()) {
        auto&& val = module_.getDomainFactory()->getTop(cast(i.getType()));
        ASSERT(val, "stub result");
        context_->state->addVariable(&i, val);
    }
}

void IRInterpreter::visitReturnInst(llvm::ReturnInst& i) {
    if (not i.getReturnValue()) return;

    auto&& retDomain = getVariable(i.getReturnValue());
    ASSERT(retDomain, "ret domain");
    context_->state->mergeToReturnValue(retDomain);
}

void IRInterpreter::visitBranchInst(llvm::BranchInst& i) {
    std::vector<BasicBlock*> successors;
    if (i.isConditional()) {
        auto cond = getVariable(i.getCondition());
        ASSERT(cond, "condition of branch");

        auto trueSuccessor = context_->function->getBasicBlock(i.getSuccessor(0));
        auto falseSuccessor = context_->function->getBasicBlock(i.getSuccessor(1));
        trueSuccessor->mergeToInput(context_->state);
        falseSuccessor->mergeToInput(context_->state);

        if (cond->isTop()) {
            auto splitted = ConditionSplitter(i.getCondition(), this, context_->state).apply();
            for (auto&& it : splitted) {
                trueSuccessor->addToInput(it.first, it.second.true_);
                falseSuccessor->addToInput(it.first, it.second.false_);
            }
            successors.emplace_back(trueSuccessor);
            successors.emplace_back(falseSuccessor);

        } else if (cond->isValue()) {
            auto boolean = llvm::dyn_cast<IntegerIntervalDomain>(cond.get());
            ASSERT(boolean && boolean->getWidth() == 1, "Non-bool in branch condition");

            if (boolean->isConstant()) {
                auto successor = boolean->isConstant(1) ?
                                 trueSuccessor :
                                 falseSuccessor;
                successors.emplace_back(successor);
            } else {
                successors.emplace_back(trueSuccessor);
                successors.emplace_back(falseSuccessor);
            }
        } else {
            successors.emplace_back(trueSuccessor);
            successors.emplace_back(falseSuccessor);
        }
    } else {
        auto successor = context_->function->getBasicBlock(i.getSuccessor(0));
        successor->mergeToInput(context_->state);
        successors.emplace_back(successor);
    }

    addSuccessors(successors);
}

void IRInterpreter::visitSwitchInst(llvm::SwitchInst& i) {
    std::vector<BasicBlock*> successors;
    auto&& cond = getVariable(i.getCondition());

    auto&& handleCaseSuccessor = [&](const llvm::BasicBlock* bb, Domain::Ptr caseValue) -> void {
        auto successor = context_->function->getBasicBlock(bb);
        successor->mergeToInput(context_->state);
        successor->addToInput(i.getCondition(), caseValue);
        successors.emplace_back(successor);
    };

    if (cond->isValue()) {
        bool isDefault = true;
        auto&& integer = llvm::dyn_cast<IntegerIntervalDomain>(cond.get());
        ASSERT(integer, "Non-integer condition in switch");

        for (auto&& cs : i.cases()) {
            auto caseVal = module_.getDomainFactory()->toInteger(cs.getCaseValue()->getValue());
            if (integer->hasIntersection(caseVal) || integer->hasSignedIntersection(caseVal)) {
                handleCaseSuccessor(cs.getCaseSuccessor(), module_.getDomainFactory()->getInteger(caseVal));
                isDefault = false;
            }
        }

        if (isDefault) handleCaseSuccessor(i.getDefaultDest(), cond);

    } else {
        for (auto&& cs : i.cases()) {
            auto caseVal = module_.getDomainFactory()->toInteger(cs.getCaseValue()->getValue());
            handleCaseSuccessor(cs.getCaseSuccessor(), module_.getDomainFactory()->getInteger(caseVal));
        }
        handleCaseSuccessor(i.getDefaultDest(), cond);
    }

    addSuccessors(successors);
}

void IRInterpreter::visitIndirectBrInst(llvm::IndirectBrInst& i) {
    std::vector<BasicBlock*> successors;

    for (auto j = 0U; j < i.getNumSuccessors(); ++j) {
        auto successor = context_->function->getBasicBlock(i.getSuccessor(j));
        successor->mergeToInput(context_->state);
        successors.emplace_back(successor);
    }
    addSuccessors(successors);
}

void IRInterpreter::visitResumeInst(llvm::ResumeInst&)             { /* ignore this */ }
void IRInterpreter::visitUnreachableInst(llvm::UnreachableInst&)   { /* ignore this */ }

void IRInterpreter::visitICmpInst(llvm::ICmpInst& i) {
    auto&& lhv = getVariable(i.getOperand(0));
    auto&& rhv = getVariable(i.getOperand(1));
    ASSERT(lhv && rhv, "cmp args");

    auto&& result = lhv->icmp(rhv, i.getPredicate());
    ASSERT(result, "cmp result");
    context_->state->addVariable(&i, result);
}

void IRInterpreter::visitFCmpInst(llvm::FCmpInst& i) {
    auto&& lhv = getVariable(i.getOperand(0));
    auto&& rhv = getVariable(i.getOperand(1));
    ASSERT(lhv && rhv, "fcmp args");

    auto&& result = lhv->fcmp(rhv, i.getPredicate());
    ASSERT(result, "fcmp result");
    context_->state->addVariable(&i, result);
}

void IRInterpreter::visitAllocaInst(llvm::AllocaInst&) {
    UNREACHABLE("Alloca instructions replaced with borealis.alloca");
}

void IRInterpreter::visitLoadInst(llvm::LoadInst& i) {
    auto gepOper = llvm::dyn_cast<llvm::GEPOperator>(i.getPointerOperand());
    auto ptr = (gepOper && not llvm::isa<llvm::GetElementPtrInst>(gepOper)) ?
               gepOperator(*gepOper) :
               getVariable(i.getPointerOperand());
    ASSERT(ptr, "load inst");

    auto&& result = ptr->load(cast(i.getType()), module_.getDomainFactory()->getIndex(0));
    ASSERT(result, "load result");
    context_->state->addVariable(&i, result);
}

void IRInterpreter::visitStoreInst(llvm::StoreInst& i) {
    auto&& ptr = getVariable(i.getPointerOperand());
    auto&& storeVal = getVariable(i.getValueOperand());
    ASSERT(ptr && storeVal, "store args");

    auto index = module_.getDomainFactory()->getIndex(0);
    if (util::contains(context_->stores, &i)) {
        auto load = ptr->load(cast(i.getValueOperand()->getType()), index);
        ptr->store(load->widen(storeVal), index);
    } else {
        ptr->store(storeVal, index);
        context_->stores.insert(&i);
    }
}

void IRInterpreter::visitGetElementPtrInst(llvm::GetElementPtrInst& i) {
    auto&& result = gepOperator(llvm::cast<llvm::GEPOperator>(i));
    ASSERT(result, "gep result");
    context_->state->addVariable(&i, result);
}

void IRInterpreter::visitPHINode(llvm::PHINode& i) {
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
    auto&& result = value->CAST(cast(i.getDestTy())); \
    ASSERT(result, "cast result"); \
    context_->state->addVariable(&i, result);

void IRInterpreter::visitTruncInst(llvm::TruncInst& i)        { CAST_INST(trunc);     }
void IRInterpreter::visitZExtInst(llvm::ZExtInst& i)          { CAST_INST(zext);      }
void IRInterpreter::visitSExtInst(llvm::SExtInst& i)          { CAST_INST(sext);      }
void IRInterpreter::visitFPTruncInst(llvm::FPTruncInst& i)    { CAST_INST(fptrunc);   }
void IRInterpreter::visitFPExtInst(llvm::FPExtInst& i)        { CAST_INST(fpext);     }
void IRInterpreter::visitFPToUIInst(llvm::FPToUIInst& i)      { CAST_INST(fptoui);    }
void IRInterpreter::visitFPToSIInst(llvm::FPToSIInst& i)      { CAST_INST(fptosi);    }
void IRInterpreter::visitUIToFPInst(llvm::UIToFPInst& i)      { CAST_INST(uitofp);    }
void IRInterpreter::visitSIToFPInst(llvm::SIToFPInst& i)      { CAST_INST(sitofp);    }
void IRInterpreter::visitPtrToIntInst(llvm::PtrToIntInst& i)  { CAST_INST(ptrtoint);  }
void IRInterpreter::visitIntToPtrInst(llvm::IntToPtrInst& i)  { CAST_INST(inttoptr);  }


void IRInterpreter::visitSelectInst(llvm::SelectInst& i) {
    auto cond = getVariable(i.getCondition());
    auto trueVal = getVariable(i.getTrueValue());
    auto falseVal = getVariable(i.getFalseValue());
    ASSERT(cond && trueVal && falseVal, "select args");

    Domain::Ptr result = cond->equals(module_.getDomainFactory()->getBool(true).get()) ? trueVal :
                         cond->equals(module_.getDomainFactory()->getBool(false).get()) ? falseVal :
                         module_.getDomainFactory()->getTop(cast(i.getType()));
    context_->state->addVariable(&i, result);
}

void IRInterpreter::visitExtractValueInst(llvm::ExtractValueInst& i) {
    auto aggregate = getVariable(i.getAggregateOperand());
    ASSERT(aggregate, "extract value arg");

    std::vector<Domain::Ptr> indices;
    for (auto j = i.idx_begin(); j != i.idx_end(); ++j) {
        indices.emplace_back(module_.getDomainFactory()->getIndex(*j));
    }

    auto result = aggregate->extractValue(cast(i.getType()), indices);
    ASSERT(result, "extract value result");
    context_->state->addVariable(&i, result);
}

void IRInterpreter::visitInsertValueInst(llvm::InsertValueInst& i) {
    auto aggregate = getVariable(i.getAggregateOperand());
    auto value = getVariable(i.getInsertedValueOperand());
    ASSERT(aggregate && value, "insert value arg");

    std::vector<Domain::Ptr> indices;
    for (auto j = i.idx_begin(); j != i.idx_end(); ++j) {
        indices.emplace_back(module_.getDomainFactory()->getIndex(*j));
    }

    aggregate->insertValue(value, indices);
}

void IRInterpreter::visitBinaryOperator(llvm::BinaryOperator& i) {
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

void IRInterpreter::visitCallInst(llvm::CallInst& i) {
    std::vector<std::pair<const llvm::Value*, Domain::Ptr>> args;
    for (auto j = 0U; j < i.getNumArgOperands(); ++j) {
        args.emplace_back(i.getArgOperand(j), getVariable(i.getArgOperand(j)));
    }
    auto retval = module_.getDomainFactory()->getBottom(cast(i.getType()));

    // inline assembler, just return TOP
    if (i.isInlineAsm()) {
        retval = module_.getDomainFactory()->getTop(cast(i.getType()));
    // function pointer
    } else if (not i.getCalledFunction()) {
        auto&& ptrDomain = getVariable(i.getCalledValue());
        ASSERT(ptrDomain, "Unknown value in call inst: " + ST_->toString(i.getCalledValue()));

        auto&& ptr = llvm::cast<PointerDomain>(ptrDomain.get());
        if (ptr->isValue()) {
            auto&& funcPtr = ptr->load(cast(i.getCalledValue()->getType()->getPointerElementType()),
                                       {module_.getDomainFactory()->getIndex(0)});
            auto&& funcDomain = llvm::cast<FunctionDomain>(funcPtr.get());

            for (auto&& func : funcDomain->getLocations()) {
                auto temp = handleFunctionCall(func->getInstance(), args);
                retval = temp ? retval->join(temp) : retval;
            }

        } else {
            warns() << "Pointer is TOP: " << ST_->toString(&i) << ", calling all possible functions" << endl;
            std::vector<llvm::Type*> argTypes;
            for (auto&& it : args) argTypes.emplace_back(it.first->getType());
            auto&& prototype = llvm::FunctionType::get(i.getType(),
                                                       llvm::ArrayRef<llvm::Type*>(argTypes),
                                                       false);
            auto&& possibleFunctions = module_.findFunctionsByPrototype(prototype);
            for (auto&& it : possibleFunctions) {
                auto temp = handleFunctionCall(it->getInstance(), args);
                retval = temp ? retval->join(temp) : retval;
            }
        }
    // usual function call
    } else {
        retval = handleFunctionCall(i.getCalledFunction(), args);
    }

    if (not i.getType()->isVoidTy()) {
        ASSERT(retval, "call inst result");
        context_->state->addVariable(&i, retval);
    }
}

void IRInterpreter::visitBitCastInst(llvm::BitCastInst& i) {
    auto&& op = getVariable(i.getOperand(0));
    ASSERT(op, "bitcast arg");

    auto&& result = op->bitcast(cast(i.getDestTy()));
    ASSERT(result, "bitcast result");
    context_->state->addVariable(&i, result);
}

///////////////////////////////////////////////////////////////
/// Util functions
///////////////////////////////////////////////////////////////
Type::Ptr IRInterpreter::cast(const llvm::Type* type) {
    return module_.getDomainFactory()->cast(type);
}

void IRInterpreter::addSuccessors(const std::vector<BasicBlock*>& successors) {
    for (auto&& it : successors) {
        if (not it->atFixpoint(module_.getGlobalsFor(it)) && not util::contains(context_->deque, it)) {
            context_->deque.emplace_back(it);
        }
    }
}

Domain::Ptr IRInterpreter::gepOperator(const llvm::GEPOperator& gep) {
    auto&& ptr = getVariable(gep.getPointerOperand());
    ASSERT(ptr, "gep args");

    std::vector<Domain::Ptr> offsets;
    for (auto j = gep.idx_begin(); j != gep.idx_end(); ++j) {
        auto val = llvm::cast<llvm::Value>(j);
        if (auto&& intConstant = llvm::dyn_cast<llvm::ConstantInt>(val)) {
            offsets.emplace_back(module_.getDomainFactory()->getIndex(*intConstant->getValue().getRawData()));
        } else if (auto indx = getVariable(val)) {
            offsets.emplace_back(indx);
        } else {
            UNREACHABLE("Non-integer constant in gep " + ST_->toString(val));
        }
    }

    return ptr->gep(cast(gep.getType()->getPointerElementType()), offsets);
}

Domain::Ptr IRInterpreter::handleFunctionCall(const llvm::Function* function,
                                            const std::vector<std::pair<const llvm::Value*, Domain::Ptr>>& args) {
    ASSERT(function, "nullptr in handle function call");
    if (function->getName().startswith("borealis.alloc") || function->getName().startswith("borealis.malloc")) {
        return handleMemoryAllocation(function, args);
    } else if (function->isDeclaration()) {
        return handleDeclaration(function, args);
    } else {
        auto func = module_.get(function);
        interpretFunction(func, util::viewContainer(args).map(LAM(a, a.second)).toVector());
        return func->getReturnValue();
    }
}

Domain::Ptr IRInterpreter::handleMemoryAllocation(const llvm::Function* function,
                                                const std::vector<std::pair<const llvm::Value*, Domain::Ptr>>& args) {
    auto&& size = getVariable(args[2].first);
    if (not size) {
        warns() << "Allocating unknown amount of memory: " << ST_->toString(args[2].first) << endl;
        return module_.getDomainFactory()->getTop(cast(function->getReturnType()));
    }

    auto&& integer = llvm::dyn_cast<IntegerIntervalDomain>(size.get());
    ASSERT(integer, "Non-integer domain in memory allocation");

    // Creating domain in memory
    // Adding new level of abstraction (pointer to array to real value), because:
    // - if this is alloc, we need one more level for correct GEP handler
    // - if this is malloc, we create array of dynamically allocated objects
    if (integer->ub()->isMax()) {
        return module_.getDomainFactory()->getTop(cast(function->getReturnType()));
    }

    auto&& arrayType = TF_->getArray(cast(function->getReturnType()->getPointerElementType()), integer->ub()->getRawValue());
    auto&& ptrType = TF_->getPointer(arrayType, 0);
    Domain::Ptr domain = module_.getDomainFactory()->allocate(ptrType);

    ASSERT(domain, "malloc result");
    return domain;
}

Domain::Ptr IRInterpreter::handleDeclaration(const llvm::Function* function,
                                           const std::vector<std::pair<const llvm::Value*, Domain::Ptr>>& args) {
    try {
        auto funcData = FIP_->getInfo(function);
        auto& argInfo = funcData.argInfo;
        for (auto j = 0U; j < args.size(); ++j) {
            auto arg = args[j].first;
            auto argType = arg->getType();
            if (argType->isPointerTy()) {
                if (argInfo[j].access == func_info::AccessPatternTag::Write ||
                        argInfo[j].access == func_info::AccessPatternTag::ReadWrite)
                    context_->state->addVariable(arg, module_.getDomainFactory()->getTop(cast(argType)));
                else if (argInfo[j].access == func_info::AccessPatternTag::Delete)
                    context_->state->addVariable(arg, module_.getDomainFactory()->getBottom(cast(argType)));
            }
        }

    } catch (std::out_of_range&) {
        // just skip llvm debug function, to prevent excess printing to log
        if (function->getName().equals("llvm.dbg.value")) return nullptr;

        warns() << "Unknown function: " << function->getName() << endl;
        // Unknown function possibly can do anything with pointer arguments
        // so we set all of them as TOP
        for (auto&& it : args) {
            auto arg = it.first;
            if (arg->getType()->isPointerTy()) {
                warns() << "Moving pointer to TOP: " << ST_->toString(arg) << endl;
                getVariable(arg)->moveToTop();
            }
        }
    }
    return module_.getDomainFactory()->getTop(cast(function->getReturnType()));
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"
