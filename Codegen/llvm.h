/*
 * llvm.h
 *
 *  Created on: Feb 25, 2013
 *      Author: ice-phoenix
 */

#ifndef CODEGEN_LLVM_H_
#define CODEGEN_LLVM_H_

#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Metadata.h>

#include "Util/locations.h"

namespace borealis {

void insertBeforeWithLocus(
        llvm::Instruction* what,
        llvm::Instruction* before,
        const Locus& loc);
void setDebugLocusWithCopiedScope(
        llvm::Instruction* to,
        llvm::Instruction* from,
        const Locus& loc);

llvm::MDNode* ptr2MDNode(llvm::LLVMContext& ctx, void* ptr);
void* MDNode2Ptr(llvm::MDNode* ptr);

llvm::StringRef getRawSource(const clang::SourceManager& sm, const LocusRange& range);

unsigned long long getTypeSizeInElems(llvm::Type* type);

} // namespace borealis

#endif /* CODEGEN_LLVM_H_ */
