/*
 * llvm.h
 *
 *  Created on: Feb 25, 2013
 *      Author: ice-phoenix
 */

#ifndef CODEGEN_LLVM_H_
#define CODEGEN_LLVM_H_

#include <llvm/Metadata.h>

namespace borealis {

llvm::MDNode* ptr2MDNode(llvm::LLVMContext& ctx, void* ptr);
void* MDNode2Ptr(llvm::MDNode* ptr);

} // namespace borealis

#endif /* CODEGEN_LLVM_H_ */
