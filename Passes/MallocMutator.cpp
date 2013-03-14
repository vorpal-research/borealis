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

void MallocMutator::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();
    AUX<llvm::TargetData>::addRequiredTransitive(AU);
}

bool MallocMutator::runOnModule(llvm::Module& M) {
    using borealis::util::viewContainer;
    using borealis::util::toString;

    llvm::TargetData& TD = GetAnalysis<llvm::TargetData>::doit(this);
    auto& intrinsic_manager = IntrinsicsManager::getInstance();

    auto* size_type = llvm::Type::getInt64Ty(M.getContext());

    std::vector<llvm::CallInst*> mallocs;
    for (llvm::Instruction& I : viewContainer(M).flatten().flatten()) {
        if (llvm::CallInst* CI = llvm::dyn_cast<llvm::CallInst>(&I)) {
            if (llvm::isMalloc(CI)) {
                mallocs.push_back(CI);
            }
        }
    }

    for (llvm::CallInst* CI : mallocs) {
        auto* arraySize = llvm::getMallocArraySize(CI, &TD, true);

        auto* current = intrinsic_manager.createIntrinsic(
                function_type::INTRINSIC_MALLOC,
                "",
                llvm::FunctionType::get(
                        CI->getType(),
                        arraySize ? arraySize->getType() : size_type,
                        false
                ),
                &M
        );

        auto* resolvedSize = arraySize && llvm::isa<llvm::Constant>(arraySize)
                ? arraySize
                : llvm::ConstantInt::get(size_type, DefaultMallocSize);

        auto* call = llvm::CallInst::Create(current, resolvedSize, "", CI);
        call->setMetadata("dbg", CI->getMetadata("dbg"));

        CI->replaceAllUsesWith(call);
        CI->eraseFromParent();
    }

    return false;
}

void MallocMutator::print(llvm::raw_ostream&, const llvm::Module*) const {}

} /* namespace borealis */
