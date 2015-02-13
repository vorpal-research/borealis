/*
 * MemoryProxy.cpp
 *
 *  Created on: Feb 12, 2015
 *      Author: ice-phoenix
 */

#include "Executor/MemoryProxy.h"

#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

using SimulatedPtr = uintptr_t*;
using SimulatedPtrSize = uint64_t;

inline SimulatedPtr ptr_cast(const uint8_t* ptr) {
    return reinterpret_cast<SimulatedPtr>(const_cast<uint8_t*>(ptr));
}

inline SimulatedPtrSize ptr_size_cast(const uint8_t* ptr) {
    return reinterpret_cast<SimulatedPtrSize>(const_cast<uint8_t*>(ptr));
}

struct MemoryProxy::Impl {

    std::unique_ptr<uint8_t[]> memory;
    SimulatedPtrSize currentOffset;

    Impl(size_t size) :
        memory(new uint8_t[size]),
        currentOffset(0) {};
    ~Impl() = default;

};

void MemoryProxy::LoadIntFromMemory(value_t& val, buffer_t where) {
    auto&& ptr = where.data();
    auto&& size = where.size();

    auto* dst = reinterpret_cast<uint8_t*>(const_cast<uint64_t*>(val.getRawData()));
    auto* src = ptr_cast(ptr);
    std::memcpy(dst, src, size);
}

void MemoryProxy::LoadBytesFromMemory(mutable_buffer_t buffer, buffer_t where) {
    ASSERTC(buffer.size() == where.size());
    auto&& ptr = where.data();
    auto&& size = where.size();

    auto* dst = buffer.data();
    auto* src = ptr_cast(ptr);
    std::memcpy(dst, src, size);
}

void MemoryProxy::StoreIntToMemory(const value_t& val, mutable_buffer_t where) {
    auto&& ptr = where.data();
    auto&& size = where.size();

    auto* dst = ptr_cast(ptr);
    auto* src = reinterpret_cast<uint8_t*>(const_cast<uint64_t*>(val.getRawData()));
    std::memcpy(dst, src, size);
}

void MemoryProxy::StoreBytesToMemory(buffer_t buffer, mutable_buffer_t where) {
    ASSERTC(buffer.size() == where.size());
    auto&& ptr = where.data();
    auto&& size = where.size();

    auto* dst = ptr_cast(ptr);
    auto* src = buffer.data();
    std::memcpy(dst, src, size);
}

void* MemoryProxy::AllocateMemory(size_t amount) {
    auto* res = pimpl_->memory.get() + pimpl_->currentOffset;
    pimpl_->currentOffset += amount;
    return res;
}

void* MemoryProxy::MallocMemory(size_t amount) {
    auto* res = pimpl_->memory.get() + pimpl_->currentOffset;
    pimpl_->currentOffset += amount;
    return res;
}

MemoryProxy::MemoryProxy(size_t size) : pimpl_(new Impl{size}) {}

MemoryProxy::~MemoryProxy() = default;

} // namespace borealis

#include "Util/unmacros.h"
