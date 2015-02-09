/*
 * MemorySimulator.h
 *
 *  Created on: Jan 28, 2015
 *      Author: belyaev
 */

#ifndef EXECUTOR_MEMORYSIMULATOR_H_
#define EXECUTOR_MEMORYSIMULATOR_H_

#include <cstdint>
#include <memory>

#include <llvm/ADT/APInt.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalValue.h>

namespace borealis {

class MemorySimulator {
    struct Impl;
    std::unique_ptr<Impl> pimpl_;

public:
    using buffer_t = llvm::ArrayRef<uint8_t>;
    using mutable_buffer_t = llvm::MutableArrayRef<uint8_t>;

    MemorySimulator(uintptr_t grain);
    ~MemorySimulator();

    enum ValueState{ UNKNOWN, CONCRETE };
    ValueState LoadIntFromMemory(llvm::APInt& val, buffer_t where);
    ValueState LoadBytesFromMemory(mutable_buffer_t buffer, buffer_t where);
    void StoreIntToMemory(const llvm::APInt& val, mutable_buffer_t where);
    void StoreBytesToMemory(buffer_t buffer, mutable_buffer_t where);

    void* AllocateMemory(size_t amount);
    enum MallocFill{ UNINIT, ZERO };
    void* MallocMemory(size_t amount, MallocFill fillWith);

    void* getPointerToFunction(llvm::Function* f, size_t size);
    void* getPointerBasicBlock(llvm::BasicBlock* bb, size_t size);
    void* getPointerToGlobal(llvm::GlobalValue* gv, size_t size);

    llvm::Function* accessFunction(void*);
    llvm::BasicBlock* accessBasicBlock(void*);
    llvm::GlobalValue* accessGlobal(void*);
};

} /* namespace borealis */

#endif /* EXECUTOR_MEMORYSIMULATOR_H_ */
