/*
 * MallocMutator.cpp
 *
 *  Created on: Mar 14, 2013
 *      Author: belyaev
 */

#include <llvm/Analysis/MemoryBuiltins.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Target/TargetData.h>

#include "Codegen/intrinsics_manager.h"
#include "Codegen/llvm.h"
#include "Passes/Transform/MallocMutator.h"
#include "Type/TypeFactory.h"
#include "Util/passes.hpp"
#include "Util/util.h"

namespace borealis {

static llvm::cl::opt<unsigned>
DefaultMallocSize("default-malloc-size", llvm::cl::init(2048), llvm::cl::NotHidden,
  llvm::cl::desc("Set default malloc size in elements (not bytes, default = 2048)"));

char MallocMutator::ID;
static RegisterPass<MallocMutator>
X("malloc-mutator", "Replace all malloc calls with borealis.malloc.*");

bool MallocMutator::canEliminateMallocBitcasts(llvm::CallInst* CI) {
    return getMallocBitcastType(CI) != nullptr;
}

llvm::PointerType* MallocMutator::getMallocBitcastType(llvm::CallInst* CI) {
    using namespace llvm;
    using borealis::util::view;

    if (CI->getNumUses() == 0) return nullptr;

    auto* bitcastType = CI->use_back()->getType();
    for (const auto* user : view(CI->use_begin(), CI->use_end())) {
        if (!isa<BitCastInst>(user)) return nullptr;
        if (bitcastType != user->getType()) return nullptr;
    }

    return dyn_cast<PointerType>(bitcastType);
}

void MallocMutator::eliminateMallocBitcasts(llvm::Module& M, llvm::CallInst* CI) {
    using namespace llvm;
    using borealis::util::toString;
    using borealis::util::view;

    auto TyF = TypeFactory::get();
    auto& TD = GetAnalysis<TargetData>::doit(this);
    auto& intrinsics_manager = IntrinsicsManager::getInstance();
    auto* size_type = llvm::Type::getInt64Ty(M.getContext());

    auto* arraySize = getMallocArraySize(CI, &TD, true);
    auto* mallocType = getMallocBitcastType(CI);
    auto elemSize = TypeUtils::getTypeSizeInElems(
        TyF->cast(mallocType->getElementType())
    );

    unsigned long long resolvedArraySize;
    if (auto* constantArraySize = dyn_cast_or_null<ConstantInt>(arraySize)) {
        resolvedArraySize = constantArraySize->getLimitedValue();
    } else {
        resolvedArraySize = DefaultMallocSize;
    }
    auto* resolvedMallocSize = ConstantInt::get(size_type, resolvedArraySize * elemSize);

    auto* current = intrinsics_manager.createIntrinsic(
            function_type::INTRINSIC_MALLOC,
            toString(*mallocType),
            FunctionType::get(
                    mallocType,
                    size_type,
                    false
            ),
            &M
    );

    auto* call = CallInst::Create(current, resolvedMallocSize, "", CI);
    call->setMetadata("dbg", CI->getMetadata("dbg"));

    // All users of this malloc call are:
    // - bitcasts
    // - of the same mallocType type

    std::vector<BitCastInst*> bitCasts;
    bitCasts.reserve(CI->getNumUses());
    for (auto* user : view(CI->use_begin(), CI->use_end())) {
        bitCasts.push_back(cast<BitCastInst>(user));
    }

    for (auto* bitCast : bitCasts) {
        bitCast->replaceAllUsesWith(call);
        bitCast->eraseFromParent();
    }

    CI->eraseFromParent();
}

void MallocMutator::mutateMalloc(llvm::Module& M, llvm::CallInst* CI) {
    using namespace llvm;
    using borealis::util::toString;
    using borealis::util::view;

    auto TyF = TypeFactory::get();
    TargetData& TD = GetAnalysis<TargetData>::doit(this);
    auto& intrinsics_manager = IntrinsicsManager::getInstance();
    auto* size_type = llvm::Type::getInt64Ty(M.getContext());

    auto* arraySize = getMallocArraySize(CI, &TD, true);
    auto* mallocType = CI->getType();
    auto elemSize = TypeUtils::getTypeSizeInElems(
        TyF->cast(getMallocAllocatedType(CI))
    );

    unsigned long long resolvedArraySize;
    if (auto* constantArraySize = dyn_cast_or_null<ConstantInt>(arraySize)) {
        resolvedArraySize = constantArraySize->getLimitedValue();
    } else {
        resolvedArraySize = DefaultMallocSize;
    }
    auto* resolvedMallocSize = ConstantInt::get(size_type, resolvedArraySize * elemSize);

    auto* current = intrinsics_manager.createIntrinsic(
            function_type::INTRINSIC_MALLOC,
            toString(*mallocType),
            llvm::FunctionType::get(
                    mallocType,
                    size_type,
                    false
            ),
            &M
    );

    auto* call = CallInst::Create(current, resolvedMallocSize, "", CI);
    call->setMetadata("dbg", CI->getMetadata("dbg"));

    CI->replaceAllUsesWith(call);
    CI->eraseFromParent();
}

void MallocMutator::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesCFG();
    AUX<llvm::TargetData>::addRequiredTransitive(AU);
}

bool MallocMutator::runOnModule(llvm::Module& M) {
    using namespace llvm;
    using borealis::util::viewContainer;
    using borealis::util::toString;

    std::vector<CallInst*> mallocs;
    for (auto& I : viewContainer(M).flatten().flatten()) {
        if (CallInst* CI = dyn_cast<CallInst>(&I)) {
            if (isMalloc(CI)) {
                mallocs.push_back(CI);
            }
        }
    }

    for (CallInst* CI : mallocs) {
        if (canEliminateMallocBitcasts(CI)) {
            eliminateMallocBitcasts(M, CI);
        } else {
            mutateMalloc(M, CI);
        }
    }

    return false;
}

void MallocMutator::print(llvm::raw_ostream&, const llvm::Module*) const {}

} /* namespace borealis */
