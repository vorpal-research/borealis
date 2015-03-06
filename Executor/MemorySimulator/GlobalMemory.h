/*
 * GlobalMemory.h
 *
 *  Created on: Mar 5, 2015
 *      Author: belyaev
 */

#ifndef EXECUTOR_MEMORYSIMULATOR_GLOBALMEMORY_H_
#define EXECUTOR_MEMORYSIMULATOR_GLOBALMEMORY_H_

#include <memory>
#include <unordered_map>
#include <cstdint>

#include <llvm/IR/GlobalVariable.h>

namespace borealis {

using global_buffer = std::unique_ptr<uint8_t[]>;
using globalMemoryTable = std::unordered_map<llvm::GlobalVariable*, global_buffer>;

static size_t calcGVRealSize(llvm::GlobalVariable* gv, const llvm::DataLayout& dl) {
    auto ElTy = gv->getType()->getElementType();
    return (size_t)dl.getTypeAllocSize(ElTy);
}

static global_buffer allocateMemoryForGV(llvm::GlobalVariable* gv, const llvm::DataLayout& dl) {
    global_buffer ret{new uint8_t[calcGVRealSize(gv, dl)]};
    return std::move(ret);
}


} /* namespace borealis */



#endif /* EXECUTOR_MEMORYSIMULATOR_GLOBALMEMORY_H_ */
