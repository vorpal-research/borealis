//
// Created by abdullin on 2/10/17.
//

#include "Annotation/Annotation.h"
#include "Config/config.h"
#include "Interpreter.h"
#include "Interpreter/Domain/Numerical/DoubleInterval.hpp"
#include "Interpreter/Domain/Memory/FunctionDomain.hpp"
#include "Util/collections.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {
namespace ir {

static config::BoolConfigEntry printModule("absint", "print-module");

Interpreter::Interpreter(const llvm::Module* module, FuncInfoProvider* FIP, SlotTrackerPass* st, CallGraphSlicer* cgs)
        : ObjectLevelLogging("ir-interpreter"), module_(module, st), TF_(TypeFactory::get()), FIP_(FIP), ST_(st), CGS_(cgs) {
    std::unordered_set<const llvm::Value*> globals;
    for (auto&& it : module->globals()) globals.insert(&it);
    module_.initGlobals(globals);
}

void Interpreter::run() {
    auto&& roots = module_.roots();
    if (roots.empty()) {
        errs() << "No root function" << endl;
        return;
    }

    if (not roots.empty()) {
        std::vector<Function::Ptr> order = util::viewContainer(CGS_->getAddressTakenFunctions()).map(LAM(a, module_.get(a))).toVector();
        for (auto&& root : roots) {
            order.emplace_back(root);
        }

        auto&& total = order.size();
        int progress = 0;
        for (auto&& root : order) {
            infos() << "Progress: " << (++progress) << " / " << total << endl;

            std::vector<AbstractDomain::Ptr> args;
            for (auto&& arg : root->getInstance()->getArgumentList()) {
                args.emplace_back(module_.variableFactory()->top(arg.getType()));
            }

            interpretFunction(root, args);
        }

    } else {

        auto&& total = module_.instance()->getFunctionList().size();
        int progress = 0;
        for (auto&& f : module_.instance()->getFunctionList()) {
            auto&& function = module_.get(&f);

            infos() << "Progress: " << (++progress) << " / " << total << endl;

            std::vector<AbstractDomain::Ptr> args;
            for (auto&& arg : function->getInstance()->getArgumentList()) {
                args.emplace_back(module_.variableFactory()->top(arg.getType()));
            }

            interpretFunction(function, args);
        }
    }

    if (printModule.get(false)) {
        infos() << endl << module_ << endl;
    }
    ASSERT(stack_.empty(), "Stack is not empty after interpretation");
    ASSERT(not context_, "Context is not empty after interpretation");
}

Module& Interpreter::getModule() {
    return module_;
}

void Interpreter::interpretFunction(Function::Ptr function, const std::vector<AbstractDomain::Ptr>& args) {
    // if arguments and globals are not changed, then this function don't need to be reinterpreted
    // if function's argument list is empty, then check if it's visited at all
    auto updArgs = function->getArguments().empty() ?
                   (not function->getEntryNode()->isVisited()) :
                   function->updateArguments(args);
    if (not updArgs) return;
    auto&& entry = function->getEntryNode();
    stack_.push({ function, nullptr, { entry }, {} });
    callStack_.insert(function);
    context_ = &stack_.top();

    while (not context_->deque.empty()) {
        auto&& basicBlock = context_->deque.front();

        // updating output block with new information
        basicBlock->mergeOutputWithInput();
        context_->state = basicBlock->getOutputState();
        visit(const_cast<llvm::BasicBlock*>(basicBlock->getInstance()));
        basicBlock->setVisited();

        // update function output block
        context_->function->merge(context_->state);

        context_->deque.pop_front();
    }
    // restore old context
    stack_.pop();
    callStack_.erase(function);
    context_ = stack_.empty() ?
               nullptr :
               &stack_.top();
}

/////////////////////////////////////////////////////////////////////
/// Visitors
/////////////////////////////////////////////////////////////////////
void Interpreter::visitInstruction(llvm::Instruction& i) {
    warns() << "Unsupported instruction: " << ST_->toString(&i) << endl;
    if (not i.getType()->isVoidTy()) {
        context_->state->assign(&i, module_.variableFactory()->top(i.getType()));
    }
}


void Interpreter::visitBranchInst(llvm::BranchInst& i) {
    std::vector<BasicBlock*> successors;
    if (i.isConditional()) {
        auto cond = context_->state->get(i.getCondition());
        ASSERT(cond, "condition of branch");

        auto trueSuccessor = context_->function->getBasicBlock(i.getSuccessor(0));
        auto falseSuccessor = context_->function->getBasicBlock(i.getSuccessor(1));

        auto&& split = context_->state->split(i.getCondition());
        auto boolean = llvm::dyn_cast<AbstractFactory::BoolT>(cond.get());

        if (boolean->isTop() || boolean->isBottom() || not boolean->isConstant()) {
            trueSuccessor->mergeToInput(split.first);
            falseSuccessor->mergeToInput(split.second);

            successors.emplace_back(trueSuccessor);
            successors.emplace_back(falseSuccessor);

        } else {
            auto successor = boolean->isConstant(1) ? trueSuccessor : falseSuccessor;
            successor->mergeToInput(boolean->isConstant(1) ? split.first : split.second);
            successors.emplace_back(successor);
        }

    } else {
        auto successor = context_->function->getBasicBlock(i.getSuccessor(0));
        successor->mergeToInput(context_->state);
        successors.emplace_back(successor);
    }

    addSuccessors(successors);
}

void Interpreter::visitSwitchInst(llvm::SwitchInst& i) {
    std::vector<BasicBlock*> successors;
    auto&& cond = context_->state->get(i.getCondition());

    auto&& handleCaseSuccessor = [&](const llvm::BasicBlock* bb, AbstractDomain::Ptr caseValue) -> void {
        auto successor = context_->function->getBasicBlock(bb);
        successor->mergeToInput(context_->state);
        successor->addToInput(i.getCondition(), caseValue);
        successors.emplace_back(successor);
    };

    if (!cond->isTop() && !cond->isBottom()) {
        bool isDefault = true;
        auto&& integer = llvm::dyn_cast<AbstractFactory::IntT>(cond.get());
        ASSERT(integer, "Non-integer condition in switch");

        for (auto&& cs : i.cases()) {
            auto caseVal = module_.variableFactory()->af()->getInteger(*cs.getCaseValue()->getValue().getRawData(), cs.getCaseValue()->getBitWidth());
            if (integer->intersects(caseVal)) {
                handleCaseSuccessor(cs.getCaseSuccessor(), caseVal);
                if (integer->isConstant()) isDefault = false;
            }
        }

        if (isDefault) handleCaseSuccessor(i.getDefaultDest(), cond);

    } else {
        for (auto&& cs : i.cases()) {
            auto caseVal = module_.variableFactory()->af()->getInteger(*cs.getCaseValue()->getValue().getRawData(), cs.getCaseValue()->getBitWidth());
            handleCaseSuccessor(cs.getCaseSuccessor(), caseVal);
        }
        handleCaseSuccessor(i.getDefaultDest(), cond);
    }

    addSuccessors(successors);
}

void Interpreter::visitIndirectBrInst(llvm::IndirectBrInst& i) {
    std::vector<BasicBlock*> successors;

    for (auto j = 0U; j < i.getNumSuccessors(); ++j) {
        auto successor = context_->function->getBasicBlock(i.getSuccessor(j));
        successor->mergeToInput(context_->state);
        successors.emplace_back(successor);
    }
    addSuccessors(successors);
}

void Interpreter::visitResumeInst(llvm::ResumeInst&)             { /* ignore this */ }
void Interpreter::visitUnreachableInst(llvm::UnreachableInst&)   { /* ignore this */ }
void Interpreter::visitReturnInst(llvm::ReturnInst&)           { /* ignore this */ }

void Interpreter::visitICmpInst(llvm::ICmpInst& i) {
    context_->state->apply(i.getPredicate(), &i, i.getOperand(0), i.getOperand(1));
}

void Interpreter::visitFCmpInst(llvm::FCmpInst& i) {
    context_->state->apply(i.getPredicate(), &i, i.getOperand(0), i.getOperand(1));
}

void Interpreter::visitAllocaInst(llvm::AllocaInst&) {
    UNREACHABLE("Alloca instructions replaced with borealis.alloca");
}

void Interpreter::visitLoadInst(llvm::LoadInst& i) {
    auto gepOper = llvm::dyn_cast<llvm::GEPOperator>(i.getPointerOperand());

    if (gepOper && not llvm::isa<llvm::GetElementPtrInst>(gepOper))
        gepOperator(*gepOper);

    context_->state->load(&i, i.getPointerOperand());
}

void Interpreter::visitStoreInst(llvm::StoreInst& i) {
    if (context_->stores.count(&i)) {
        context_->state->storeWithWidening(i.getPointerOperand(), i.getValueOperand());
    } else {
        context_->state->store(i.getPointerOperand(), i.getValueOperand());
        context_->stores.insert(&i);
    }
}

void Interpreter::visitGetElementPtrInst(llvm::GetElementPtrInst& i) {
    gepOperator(llvm::cast<llvm::GEPOperator>(i));
}

void Interpreter::visitPHINode(llvm::PHINode& i) {
    AbstractDomain::Ptr result = module_.variableFactory()->bottom(i.getType());

    bool changed = false;
    if (not context_->state->get(&i)) {
        for (auto j = 0U; j < i.getNumIncomingValues(); ++j) {
            auto&& predecessor = context_->function->getBasicBlock(i.getIncomingBlock(j));
            if (predecessor->isVisited()) {
                auto&& incoming = context_->state->get(i.getIncomingValue(j));
                if (not incoming) continue;

                result = result->join(incoming);
                changed = true;
            }
        }

    } else {
        for (auto j = 0U; j < i.getNumIncomingValues(); ++j) {
            auto&& predecessor = context_->function->getBasicBlock(i.getIncomingBlock(j));
            if (predecessor->isVisited()) {
                auto&& incoming = context_->state->get(i.getIncomingValue(j));
                if (not incoming) continue;

                result = result->widen(incoming);
                changed = true;
            }
        }
    }

    ASSERT(result, "phi result");
    ASSERTC(changed);
    context_->state->assign(&i, result);
}

#define CAST_INST(CAST) \
    context_->state->apply((CAST), &i, i.getOperand(0));

void Interpreter::visitTruncInst(llvm::TruncInst& i)        { CAST_INST(CastOperator::TRUNC);   }
void Interpreter::visitZExtInst(llvm::ZExtInst& i)          { CAST_INST(CastOperator::EXT);     }
void Interpreter::visitSExtInst(llvm::SExtInst& i)          { CAST_INST(CastOperator::SEXT);    }
void Interpreter::visitFPTruncInst(llvm::FPTruncInst& i)    { CAST_INST(CastOperator::TRUNC);   }
void Interpreter::visitFPExtInst(llvm::FPExtInst& i)        { CAST_INST(CastOperator::EXT);     }
void Interpreter::visitFPToUIInst(llvm::FPToUIInst& i)      { CAST_INST(CastOperator::FPTOI);   }
void Interpreter::visitFPToSIInst(llvm::FPToSIInst& i)      { CAST_INST(CastOperator::FPTOI);   }
void Interpreter::visitUIToFPInst(llvm::UIToFPInst& i)      { CAST_INST(CastOperator::ITOFP);   }
void Interpreter::visitSIToFPInst(llvm::SIToFPInst& i)      { CAST_INST(CastOperator::ITOFP);  }
void Interpreter::visitPtrToIntInst(llvm::PtrToIntInst& i)  { CAST_INST(CastOperator::PTRTOI);  }
void Interpreter::visitIntToPtrInst(llvm::IntToPtrInst& i)  { CAST_INST(CastOperator::ITOPTR);  }
void Interpreter::visitBitCastInst(llvm::BitCastInst& i)    { CAST_INST(CastOperator::BITCAST); }


void Interpreter::visitSelectInst(llvm::SelectInst& i) {
    auto cond = context_->state->get(i.getCondition());
    auto trueVal = context_->state->get(i.getTrueValue());
    auto falseVal = context_->state->get(i.getFalseValue());
    ASSERT(cond && trueVal && falseVal, "select args");

    auto&& result = cond->equals(module_.variableFactory()->af()->getBool(true)) ? trueVal :
                         cond->equals(module_.variableFactory()->af()->getBool(false)) ? falseVal :
                         module_.variableFactory()->top(i.getType());
    context_->state->assign(&i, result);
}

void Interpreter::visitBinaryOperator(llvm::BinaryOperator& i) {
    context_->state->apply(i.getOpcode(), &i, i.getOperand(0), i.getOperand(1));
}

void Interpreter::visitCallInst(llvm::CallInst& i) {
    // inline assembler, just return TOP
    if (i.isInlineAsm()) {
        context_->state->assign(&i, module_.variableFactory()->top(i.getType()));
    // function pointer
    } else if (not i.getCalledFunction()) {
        std::vector<std::pair<const llvm::Value*, AbstractDomain::Ptr>> args;
        for (auto j = 0U; j < i.getNumArgOperands(); ++j) {
            args.emplace_back(i.getArgOperand(j), context_->state->get(i.getArgOperand(j)));
        }

        auto&& ptrDomain = context_->state->get(i.getCalledValue());
        ASSERT(ptrDomain, "Unknown value in call inst: " + ST_->toString(i.getCalledValue()));

        auto&& functionType = module_.variableFactory()->cast(i.getCalledValue()->getType()->getPointerElementType());
        auto&& funcDomain = ptrDomain->load(functionType, module_.variableFactory()->af()->getMachineInt(0));
        auto&& function = llvm::cast<FunctionDomain<Function::Ptr, FunctionHash, FunctionEquals>>(funcDomain.get());

        auto retval = module_.variableFactory()->bottom(i.getType());
        if (function->isTop() || function->isBottom()) {
//            warns() << "Pointer is TOP: " << ST_->toString(&i) << ", calling all possible functions" << endl;
            std::vector<llvm::Type*> argTypes;
            for (auto&& it : args) argTypes.emplace_back(it.first->getType());
            auto&& prototype = llvm::FunctionType::get(i.getType(),
                                                       llvm::ArrayRef<llvm::Type*>(argTypes),
                                                       false);
            auto&& possibleFunctions = module_.findByPrototype(prototype);
            for (auto&& it : possibleFunctions) {
                auto temp = handleFunctionCall(it->getInstance(), &i, args);
                retval = temp ? retval->join(temp) : retval;
            }

        } else {
            for (auto&& func : function->functions()) {
                auto temp = handleFunctionCall(func->getInstance(), &i, args);
                retval = temp ? retval->join(temp) : retval;
            }

        }

        if (retval)
            context_->state->assign(&i, retval);
    // usual function call
    } else {
//        if (i.getCalledFunction()->getName().startswith("llvm.")) return;

        std::vector<std::pair<const llvm::Value*, AbstractDomain::Ptr>> args;
        for (auto j = 0U; j < i.getNumArgOperands(); ++j) {
            args.emplace_back(i.getArgOperand(j), context_->state->get(i.getArgOperand(j)));
        }

        auto retval = handleFunctionCall(i.getCalledFunction(), &i, args);
        if (retval)
            context_->state->assign(&i, retval);
    }
}

///////////////////////////////////////////////////////////////
/// Util functions
///////////////////////////////////////////////////////////////
void Interpreter::addSuccessors(const std::vector<BasicBlock*>& successors) {
    for (auto&& it : successors) {
        auto&& atFixpoint = it->atFixpoint();
        auto&& inQueue = util::contains(context_->deque, it);
        if (not atFixpoint && not inQueue) {
            context_->deque.emplace_back(it);
        }
    }
}

void Interpreter::gepOperator(const llvm::GEPOperator& gep) {
    std::vector<const llvm::Value*> shifts;
    for (auto j = gep.idx_begin(); j != gep.idx_end(); ++j) {
        auto val = llvm::cast<llvm::Value>(j);
        shifts.push_back(val);
    }

    auto* ptr = gep.getPointerOperand();
    if (llvm::isa<llvm::GEPOperator>(ptr) && not llvm::isa<llvm::GetElementPtrInst>(ptr))
        gepOperator(*llvm::cast<llvm::GEPOperator>(ptr));

    context_->state->gep(&gep, ptr, shifts);
}

AbstractDomain::Ptr Interpreter::handleFunctionCall(
        const llvm::Function* function,
        const llvm::Value* result,
        const std::vector<std::pair<const llvm::Value*, AbstractDomain::Ptr>>& args) {
    ASSERT(function, "Nullptr in function call");

    if (function->getName().startswith("borealis.alloc") || function->getName().startswith("borealis.malloc")) {
        context_->state->allocate(result, args[2].first);
        return context_->state->get(result);
    } else if (function->isDeclaration()) {
        return handleDeclaration(function, result, args);
    } else {
        auto func = module_.get(function);
        if (callStack_.count(func))
            return module_.variableFactory()->top(func->getInstance()->getReturnType());
        interpretFunction(func, util::viewContainer(args).map(LAM(a, a.second)).toVector());
        return func->getReturnValue();
    }
}

AbstractDomain::Ptr Interpreter::handleDeclaration(const llvm::Function* function,
                                           const llvm::Value*,
                                           const std::vector<std::pair<const llvm::Value*, AbstractDomain::Ptr>>& args) {
    try {
        auto funcData = FIP_->getInfo(function);
        auto& argInfo = funcData.argInfo;
        for (auto j = 0U; j < args.size(); ++j) {
            auto arg = args[j].first;
            auto argType = arg->getType();
            if (argType->isPointerTy()) {
                if (argInfo[j].access == func_info::AccessPatternTag::Write ||
                        argInfo[j].access == func_info::AccessPatternTag::ReadWrite)
                    context_->state->assign(arg, module_.variableFactory()->top(argType));
                else if (argInfo[j].access == func_info::AccessPatternTag::Delete)
                    context_->state->assign(arg, module_.variableFactory()->bottom(argType));
            }
        }

    } catch (std::out_of_range&) {
        // just skip llvm debug function, to prevent excess printing to log
        if (function->getName().startswith("llvm.dbg.")) return nullptr;

//        warns() << "Unknown function: " << function->getName() << endl;
        // Unknown function possibly can do anything with pointer arguments
        // so we set all of them as TOP
        for (auto&& it : args) {
            auto arg = it.first;
            if (arg->getType()->isPointerTy()) {
                context_->state->get(arg)->setTop();
            }
        }
    }
    return module_.variableFactory()->top(function->getReturnType());
}

}   /* namespace ir */
}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"
