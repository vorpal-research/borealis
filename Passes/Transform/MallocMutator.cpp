/*
 * MallocMutator.cpp
 *
 *  Created on: Mar 14, 2013
 *      Author: belyaev
 */

#include <llvm/Analysis/MemoryBuiltins.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/Target/TargetLibraryInfo.h>

#include "Config/config.h"
#include "Codegen/intrinsics_manager.h"
#include "Codegen/llvm.h"
#include "Passes/Transform/MallocMutator.h"
#include "Type/TypeFactory.h"
#include "Util/passes.hpp"
#include "Util/util.h"

namespace borealis {

char MallocMutator::ID;
static RegisterPass<MallocMutator>
X("malloc-mutator", "Replace all (m)alloc calls with borealis.(m)alloc.*");

llvm::Type* getMallocBitcastType(llvm::CallInst* CI) {
    using namespace llvm;

    if (CI->getNumUses() == 0) return nullptr;

    auto* bitcastType = CI->user_back()->getType();
    for (const auto* user : util::view(CI->user_begin(), CI->user_end())) {
        if (!isa<BitCastInst>(user)) return nullptr;
        if (bitcastType != user->getType()) return nullptr;
    }

    return bitcastType;
}

bool canEliminateMallocBitcasts(llvm::CallInst* CI) {
    return getMallocBitcastType(CI) != nullptr;
}

llvm::Value* getMallocArraySizeOrArg(
        llvm::CallInst* CI,
        const llvm::DataLayout* TD,
        const llvm::TargetLibraryInfo* TLI) {
    auto* arraySize = llvm::getMallocArraySize(CI, TD, TLI, true);
    return arraySize ? arraySize : CI->getArgOperand(0);
}

void eliminateMallocBitcasts(llvm::Instruction* old_, llvm::Instruction* new_) {
    using namespace llvm;

    // All users of old_ malloc call are:
    // - bitcasts
    // - of the same type

    auto bitCasts = util::view(old_->user_begin(), old_->user_end())
                    .map(dyn_caster<BitCastInst>())
                    .filter()
                    .toVector();

    for (auto* bitCast : bitCasts) {
        bitCast->replaceAllUsesWith(new_);
        bitCast->eraseFromParent();
    }

    old_->eraseFromParent();
}

void mutateMalloc(llvm::Instruction* old_, llvm::Instruction* new_) {
    old_->replaceAllUsesWith(new_);
    old_->eraseFromParent();
}

void mutateAlloc(llvm::Instruction* old_, llvm::Instruction* new_) {
    old_->replaceAllUsesWith(new_);
    old_->eraseFromParent();
}

void MallocMutator::mutateMemoryInst(
    llvm::Module& M,
    llvm::Instruction* CI,
    function_type memoryType,
    llvm::Type* resultType,
    llvm::Type* elemType,
    llvm::Value* arraySize,
    std::function<void(llvm::Instruction*, llvm::Instruction*)> mutator
) {
    static config::ConfigEntry<int> DefaultMallocSizeOpt("analysis", "default-malloc-size");
    unsigned DefaultMallocSize = DefaultMallocSizeOpt.get(2048);

    using namespace llvm;

    static auto TyF = TypeFactory::get();
    auto& intrinsics_manager = IntrinsicsManager::getInstance();

    auto* sizeType = llvm::Type::getInt64Ty(M.getContext());

    auto elemSize = TypeUtils::getTypeSizeInElems(
        TyF->cast(elemType, M.getDataLayout())
    );

    auto* resolvedElemSize = ConstantInt::get(sizeType, elemSize);

    auto* originalArraySize = arraySize;

    unsigned long long resolvedArraySize;
    if (auto* constantArraySize = dyn_cast_or_null<ConstantInt>(arraySize)) {
        resolvedArraySize = constantArraySize->getLimitedValue();
    } else {
        resolvedArraySize = DefaultMallocSize;
    }
    auto* resolvedMallocSize = ConstantInt::get(sizeType, resolvedArraySize * elemSize);

    auto* current = intrinsics_manager.createIntrinsic(
            memoryType,
            util::toString(*resultType),
            llvm::FunctionType::get(
                resultType,
                llvm::makeArrayRef<llvm::Type*>({
                    resolvedMallocSize->getType(),
                    resolvedElemSize->getType(),
                    originalArraySize->getType()
                }),
                false
            ),
            &M
    );

    auto* call = CallInst::Create(
        current,
        llvm::makeArrayRef<llvm::Value*>({ resolvedMallocSize, resolvedElemSize, originalArraySize }),
        "",
        CI
    );
    call->setMetadata("dbg", CI->getMetadata("dbg"));

    mutator(CI, call);
}

void MallocMutator::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesCFG();
    AUX<llvm::DataLayoutPass>::addRequiredTransitive(AU);
    AUX<llvm::TargetLibraryInfo>::addRequiredTransitive(AU);
}

bool MallocMutator::runOnModule(llvm::Module& M) {
    using namespace llvm;
    using borealis::util::viewContainer;
    using borealis::util::toString;

    auto& TD = GetAnalysis<DataLayoutPass>::doit(this).getDataLayout();
    auto& TLI = GetAnalysis<TargetLibraryInfo>::doit(this);


    std::vector<CallInst*> mallocs;
    std::vector<AllocaInst*> allocas;
    for (auto& I : viewContainer(M).flatten().flatten()) {
        if (CallInst* CI = dyn_cast<CallInst>(&I)) {
            if (isMallocLikeFn(CI, &TLI, true)) {
                mallocs.push_back(CI);
            }
        }

        if (AllocaInst* AL = dyn_cast<AllocaInst>(&I)) {
            allocas.push_back(AL);
        }
    }

    for (CallInst* CI : mallocs) {
        if (canEliminateMallocBitcasts(CI)) {
            auto* resType = getMallocBitcastType(CI);
            mutateMemoryInst(
                M,
                CI,
                function_type::INTRINSIC_MALLOC,
                resType,
                resType->getPointerElementType(),
                getMallocArraySizeOrArg(CI, &TD, &TLI),
                eliminateMallocBitcasts
            );
        } else {
            auto* resType = CI->getType();
            mutateMemoryInst(
                M,
                CI,
                function_type::INTRINSIC_MALLOC,
                resType,
                resType->getPointerElementType(),
                getMallocArraySizeOrArg(CI, &TD, &TLI),
                mutateMalloc
            );
        }
    }

    for (AllocaInst* AL : allocas) {
        mutateMemoryInst(
            M,
            AL,
            function_type::INTRINSIC_ALLOC,
            AL->getType(),
            AL->getAllocatedType(),
            AL->getArraySize(),
            mutateAlloc
        );
    }

    return false;
}

void MallocMutator::print(llvm::raw_ostream&, const llvm::Module*) const {}

} /* namespace borealis */
