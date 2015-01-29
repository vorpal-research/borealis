/*
 * MemorySimulator.h
 *
 *  Created on: Jan 28, 2015
 *      Author: belyaev
 */

#ifndef EXECUTOR_MEMORYSIMULATOR_H_
#define EXECUTOR_MEMORYSIMULATOR_H_

#include <memory>
#include <cstdint>

#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/ADT/APInt.h>

namespace borealis {

class MemorySimulator {
    struct Impl;
    std::unique_ptr<Impl> pimpl_;

public:
    MemorySimulator();
    ~MemorySimulator();

    void LoadIntFromMemory(llvm::APInt& val, uint8_t* Ptr, size_t size);
    void StoreIntToMemory(const llvm::APInt& val, uint8_t* Ptr, size_t size);
    void* AllocateMemory(size_t amount);

    void* getPointerToFunction(const llvm::Function* f);
    void* getPointerBasicBlock(const llvm::BasicBlock* f);
    void* getPointerToGlobal(const llvm::GlobalValue* gv);
};

} /* namespace borealis */

#endif /* EXECUTOR_MEMORYSIMULATOR_H_ */
