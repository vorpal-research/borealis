/*
 * UglyGEPKiller.cpp
 *
 *  Created on: Jan 28, 2014
 *      Author: belyaev
 */

#include <array>

#include <llvm/IR/TypeBuilder.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/Casting.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/PatternMatch.h>
#include <llvm/IR/DataLayout.h>

#include "Passes/Transform/UglyGEPKiller.h"

#include "Util/llvm_patterns.hpp"
#include "Util/passes.hpp"

namespace borealis {

void UglyGEPKiller::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesCFG();
    AU.addRequired<llvm::DataLayoutPass>();
}

static llvm::SmallVector<llvm::Value*, 2> tryFindAdjustedIndices(
    llvm::Value* idx,
    llvm::Type* dePointed,
    const llvm::DataLayout* TD,
    llvm::IRBuilder<>& builder
) {
    using namespace llvm;
    // Check to see if "tmp" is a scale by a multiple of ArrayEltSize. We
    // allow either a mul, shift, or constant here.
    Value* NewIdx = nullptr;
    ConstantInt* Scale = nullptr;
    Type* type = nullptr;
    bool isArray = false;

    if(auto arrayType = llvm::dyn_cast<ArrayType>(dePointed)) {
        type = arrayType->getArrayElementType();
        isArray = true;
    } else if( dePointed->isSingleValueType() ) {
        type = dePointed;
    }
    if(!type) return {};

    uint64_t eltSize = TD->getTypeAllocSize(type);

    if (eltSize == 1) {
        NewIdx = idx;
        Scale = ConstantInt::get(cast<IntegerType>(NewIdx->getType()), 1);
    } else if (ConstantInt *CI = dyn_cast<ConstantInt>(idx)) {
        NewIdx = ConstantInt::get(CI->getType(), 1);
        Scale = CI;
    } else if (Instruction *Inst = dyn_cast<Instruction>(idx)){
        if (Inst->getOpcode() == Instruction::Shl &&
            isa<ConstantInt>(Inst->getOperand(1))) {
            ConstantInt *ShAmt = cast<ConstantInt>(Inst->getOperand(1));
            uint32_t ShAmtVal = ShAmt->getLimitedValue(64);
            Scale = ConstantInt::get(cast<IntegerType>(Inst->getType()),
                1ULL << ShAmtVal);
            NewIdx = Inst->getOperand(0);
        } else if (Inst->getOpcode() == Instruction::Mul &&
            isa<ConstantInt>(Inst->getOperand(1))) {
            Scale = cast<ConstantInt>(Inst->getOperand(1));
            NewIdx = Inst->getOperand(0);
        }
    }

    auto iScale = Scale->getZExtValue();

    if(iScale % eltSize != 0) return {};

    Scale  = ConstantInt::get(Scale->getType(), iScale / eltSize);
    iScale = Scale->getZExtValue();

    if (iScale != 1) {
        Constant *C = ConstantExpr::getIntegerCast(Scale, NewIdx->getType(), false /*ZExt*/);
        NewIdx = builder.CreateMul(NewIdx, C, "idxscale");
    }

    if(isArray) {
        std::array<llvm::Value*, 2> tmp {{ ConstantInt::getNullValue(Type::getInt32Ty(idx->getContext())), NewIdx }};
        return { std::begin(tmp), std::end(tmp) };
    } else {
        std::array<llvm::Value*, 1> tmp {{ NewIdx }};
        return { std::begin(tmp), std::end(tmp) };
    }
}


bool UglyGEPKiller::runOnModule(llvm::Module& M) {
    using namespace llvm;
    using namespace llvm::PatternMatch;

    // the code gracefully stolen from SimplifyGep pass in poolalloc
    auto TD = &getAnalysis<llvm::DataLayoutPass>().getDataLayout();
    for(auto& I : util::viewContainer(M).flatten().flatten()) {
        Value* idx = nullptr;
        Value* srcPtr = nullptr;
        if(
            !match(
                &I,
                m_BitCast(
                    m_GetElementPtrInst(
                        m_WithTypeTemplate< types::i<8>* >(
                            m_BitCast(
                                m_Value(srcPtr)
                            )
                        ),
                        m_Value(idx)
                    )
                )
            )
        ) continue;

        if(!idx || !srcPtr) continue; // should neva happen

        llvm::Type* startType = srcPtr->getType();
        llvm::Type* endType = I.getType();

        if( ! startType->isPointerTy()
         || ! endType->isPointerTy() ) continue;

        startType = startType->getPointerElementType();
        endType   = endType->getPointerElementType();

        IRBuilder<> builder{ &I };
        llvm::Value* res = nullptr;

        llvm::SmallVector<llvm::Value*, 2> newIdxs = tryFindAdjustedIndices(idx, startType, TD, builder);
        if(!newIdxs.empty()) {
            auto* tmp = builder.CreateGEP(srcPtr, newIdxs, "bor.reformed.uglygep");
            res = builder.CreateBitCast(tmp, endType -> getPointerTo(), "bor.reformed.uglygep.cast");
        } else {
            newIdxs = tryFindAdjustedIndices(idx, endType, TD, builder);
            if(newIdxs.empty()) continue;

            auto* tmp = builder.CreateBitCast(srcPtr, endType -> getPointerTo(), "bor.reformed.uglygep.cast");
            res = builder.CreateGEP(tmp, newIdxs, "bor.reformed.uglygep");
        }

        I.replaceAllUsesWith(res);
    }

    return true;
}


void UglyGEPKiller::print(llvm::raw_ostream&, const llvm::Module*) const {}

static RegisterPass<UglyGEPKiller> X {
    "kill-uglygeps",
    "A pass that tries really hard to find all uglygeps (geps with byte arithmetic) and kill them "
};
char UglyGEPKiller::ID = 47; // codename 47

} /* namespace borealis */
