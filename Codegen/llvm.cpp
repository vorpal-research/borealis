/*
 * llvm.cpp
 *
 *  Created on: Feb 25, 2013
 *      Author: ice-phoenix
 */

#include <llvm/Constants.h>

#include "Codegen/llvm.h"

namespace borealis {

llvm::MDNode* ptr2MDNode(llvm::LLVMContext& ctx, void* ptr) {
    llvm::Constant* ptrAsInt = llvm::ConstantInt::get(
            ctx,
            llvm::APInt(sizeof(uintptr_t)*8, reinterpret_cast<uintptr_t>(ptr))
    );
    return llvm::MDNode::get(ctx, ptrAsInt);
}

void* MDNode2Ptr(llvm::MDNode* ptr) {
    if (ptr)
        if (auto* i = llvm::dyn_cast<llvm::ConstantInt>(ptr->getOperand(0)))
            return reinterpret_cast<void*>(static_cast<uintptr_t>(i->getLimitedValue()));
    return nullptr;
}

} // namespace borealis
