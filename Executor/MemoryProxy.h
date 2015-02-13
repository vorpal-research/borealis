/*
 * MemoryProxy.h
 *
 *  Created on: Feb 12, 2015
 *      Author: ice-phoenix
 */

#ifndef EXECUTOR_MEMORYPROXY_H_
#define EXECUTOR_MEMORYPROXY_H_

#include <llvm/ADT/APInt.h>
#include <llvm/ADT/ArrayRef.h>

#include <memory>

namespace borealis {

class MemoryProxy {
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
public:
    using value_t = llvm::APInt;
    using buffer_t = llvm::ArrayRef<uint8_t>;
    using mutable_buffer_t = llvm::MutableArrayRef<uint8_t>;

    MemoryProxy(size_t size);
    ~MemoryProxy();

    void LoadIntFromMemory(value_t& val, buffer_t where);
    void LoadBytesFromMemory(mutable_buffer_t buffer, buffer_t where);

    void StoreIntToMemory(const value_t& val, mutable_buffer_t where);
    void StoreBytesToMemory(buffer_t buffer, mutable_buffer_t where);

    void* AllocateMemory(size_t amount);

    void* MallocMemory(size_t amount);

};

} // namespace borealis

#endif /* EXECUTOR_MEMORYPROXY_H_ */
