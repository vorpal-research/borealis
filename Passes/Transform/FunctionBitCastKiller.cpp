//
// Created by abdullin on 10/17/17.
//

#include <unistd.h>
#include "Codegen/llvm.h"
#include "FunctionBitCastKiller.h"
#include "Util/collections.hpp"
#include "Util/functional.hpp"
#include "Util/passes.hpp"

#include "Util/macros.h"

namespace borealis {

void FunctionBitCastKiller::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    llvm::Pass::getAnalysisUsage(AU);
}

bool FunctionBitCastKiller::runOnFunction(llvm::Function& F) {
    deleted_instructions_.clear();

    for (auto&& I : util::viewContainer(F)
            .flatten()
            .map(ops::take_pointer)
            .map(llvm::dyn_caster<llvm::CallInst>())
            .filter()) {
        visitCallInst(*I);
    }

    for (auto&& i : util::viewContainer(deleted_instructions_)
            .map(llvm::dyn_caster<llvm::Instruction>())
            .filter()) {
        for (auto&& user: util::viewContainer(i->users()).toVector()) {
            if (auto&& use = llvm::dyn_cast<llvm::Instruction>(user)) {
                use->eraseFromParent();
            }
        }
        i->eraseFromParent();
    }
    return true;
}

void FunctionBitCastKiller::visitCallInst(llvm::CallInst& call) {
    if (call.isInlineAsm() || call.getCalledFunction()) return;
    auto&& ce = llvm::dyn_cast<llvm::ConstantExpr>(call.getCalledValue());
    if (not ce || ce->getOpcode() != llvm::Instruction::BitCast) return;

    auto&& function = llvm::cast<llvm::Function>(ce->getOperand(0));
    auto&& functionType = llvm::cast<llvm::FunctionType>(function->getType()->getPointerElementType());

    std::vector<llvm::Value*> newArgs;
    auto j = 0U;
    for (; j < functionType->getNumParams(); ++j) {
        auto arg = call.getArgOperand(j);
        auto newarg = generateCastInst(arg, functionType->getParamType(j));
        if (newarg) {
            newarg->insertBefore(&call);
            newArgs.emplace_back(newarg);
        } else {
            newArgs.emplace_back(arg);
        }
    }
    for (; j < call.getNumArgOperands(); ++j) newArgs.emplace_back(call.getArgOperand(j));
    ASSERT(newArgs.size() == functionType->getNumParams() ||
                   (functionType->isVarArg() && newArgs.size() > functionType->getNumParams()),
           "Incorrect bitcasting of function: " + function->getName().str());

    auto newCall = llvm::CallInst::Create(function, newArgs);
    newCall->insertBefore(&call);
    copyMetadata(call, *newCall);
    if (newCall->getType() != call.getType()) {
        auto result = generateCastInst(newCall, call.getType());
        result->insertBefore(&call);
        call.replaceAllUsesWith(result);
    } else {
        call.replaceAllUsesWith(newCall);
    }
    deleted_instructions_.emplace(&call);
}

void FunctionBitCastKiller::copyMetadata(const llvm::Instruction &from, llvm::Instruction &to) {
    llvm::SmallVector<std::pair<unsigned, llvm::MDNode*>, 0> metadata;
    from.getAllMetadata(metadata);

    for (auto&& it: metadata) {
        to.setMetadata(it.first, it.second);
    }
}

char FunctionBitCastKiller::ID = 0;

llvm::Instruction* FunctionBitCastKiller::generateCastInst(llvm::Value* val, llvm::Type* to) {
    auto valType = val->getType();
    if (valType == to) return nullptr;
    if (valType->isPointerTy() && to->isPointerTy()) {
        return new llvm::BitCastInst(val, to);
    } else if (valType->isPointerTy() && to->isIntegerTy()) {
        return new llvm::PtrToIntInst(val, to);
    } else if (valType->isAggregateType() && to->isAggregateType()) {
        return new llvm::PtrToIntInst(val, to);
    } else if (valType->isIntegerTy() && to->isIntegerTy()) {
        auto valIntType = llvm::cast<llvm::IntegerType>(valType);
        auto toIntType = llvm::cast<llvm::IntegerType>(to);
        if (valIntType->getBitWidth() > toIntType->getBitWidth()) return new llvm::TruncInst(val, to);
        else return new llvm::ZExtInst(val, to);
    } else if (valType->isFloatingPointTy() && to->isIntegerTy()) {
        return new llvm::FPToUIInst(val, to);
    } else if (valType->isIntegerTy() && to->isFloatingPointTy()) {
        return new llvm::UIToFPInst(val, to);
    }
    UNREACHABLE("Unknown casting: from = " + util::toString(*valType) + "; to = " + util::toString(*to));
}

static llvm::RegisterPass<FunctionBitCastKiller>
X("function-bitcast-killer", "Pass that kills all function bitcasts in call instructions", false, false);

}   // namespace borealis

#include "Util/unmacros.h"