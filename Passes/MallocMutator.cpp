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
#include "Passes/MallocMutator.h"
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
    using llvm::Type;
    using borealis::util::view;

    if (CI->getNumUses() == 0) return nullptr;

    Type* bitcastType = CI->use_back()->getType();
    for (const User* user : view(CI->use_begin(), CI->use_end())) {
        if (!isa<BitCastInst>(user)) return nullptr;
        if (bitcastType != user->getType()) return nullptr;
    }

    return dyn_cast<PointerType>(bitcastType);
}

void MallocMutator::eliminateMallocBitcasts(llvm::Module& M, llvm::CallInst* CI) {
    using namespace llvm;
    using llvm::Type;
    using borealis::util::toString;
    using borealis::util::view;

    auto& TD = GetAnalysis<TargetData>::doit(this);
    auto& intrinsic_manager = IntrinsicsManager::getInstance();
    auto* size_type = Type::getInt64Ty(M.getContext());

    auto* arraySize = getMallocArraySize(CI, &TD, true);
    auto* mallocType = getMallocBitcastType(CI);
    auto elemSize = getTypeSizeInElems(mallocType->getElementType());

    unsigned long long resolvedArraySize;
    if (auto* constantArraySize = dyn_cast_or_null<ConstantInt>(arraySize)) {
        resolvedArraySize = constantArraySize->getZExtValue();
    } else {
        resolvedArraySize = DefaultMallocSize;
    }
    auto* resolvedMallocSize = ConstantInt::get(size_type, resolvedArraySize * elemSize);

    auto* current = intrinsic_manager.createIntrinsic(
            function_type::INTRINSIC_MALLOC,
            toString(mallocType),
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
    for (User* user : view(CI->use_begin(), CI->use_end())) {
        BitCastInst* bitCast = cast<BitCastInst>(user);
        bitCast->replaceAllUsesWith(call);
        bitCast->eraseFromParent();
    }
    CI->eraseFromParent();
}

void MallocMutator::mutateMalloc(llvm::Module& M, llvm::CallInst* CI) {
    using namespace llvm;
    using llvm::Type;
    using borealis::util::toString;
    using borealis::util::view;

    TargetData& TD = GetAnalysis<TargetData>::doit(this);
    auto& intrinsic_manager = IntrinsicsManager::getInstance();
    auto* size_type = Type::getInt64Ty(M.getContext());

    auto* arraySize = getMallocArraySize(CI, &TD, true);
    auto* mallocType = CI->getType();
    auto elemSize = getTypeSizeInElems(getMallocAllocatedType(CI));

    unsigned long long resolvedArraySize;
    if (auto* constantArraySize = dyn_cast_or_null<ConstantInt>(arraySize)) {
        resolvedArraySize = constantArraySize->getZExtValue();
    } else {
        resolvedArraySize = DefaultMallocSize;
    }
    auto* resolvedMallocSize = ConstantInt::get(size_type, resolvedArraySize * elemSize);

    auto* current = intrinsic_manager.createIntrinsic(
            function_type::INTRINSIC_MALLOC,
            toString(mallocType),
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
    AU.setPreservesAll();
    AUX<llvm::TargetData>::addRequiredTransitive(AU);
}

bool MallocMutator::runOnModule(llvm::Module& M) {
    using borealis::util::viewContainer;
    using borealis::util::toString;

    std::vector<llvm::CallInst*> mallocs;
    for (llvm::Instruction& I : viewContainer(M).flatten().flatten()) {
        if (llvm::CallInst* CI = llvm::dyn_cast<llvm::CallInst>(&I)) {
            if (llvm::isMalloc(CI)) {
                mallocs.push_back(CI);
            }
        }
    }

    for (llvm::CallInst* CI : mallocs) {
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
