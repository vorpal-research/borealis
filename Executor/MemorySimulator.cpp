/*
 * MemorySimulator.cpp
 *
 *  Created on: Jan 28, 2015
 *      Author: belyaev
 */

#include <llvm/Support/Host.h>

#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <map>

#include <tinyformat/tinyformat.h>

#include "Executor/MemorySimulator.h"
#include "Executor/Exceptions.h"
#include "Util/util.h"
#include "Util/collections.hpp"
#include "Config/config.h"

#include "Executor/MemorySimulator/SegmentTreeImpl.h"
#include "Executor/MemorySimulator/GlobalMemory.h"

#include "Logging/tracer.hpp"

#include "Util/macros.h"

namespace borealis {

struct MemorySimulator::Impl {
    SegmentTree tree;
    std::unordered_map<llvm::Value*, SimulatedPtr> constants;
    std::map<SimulatedPtr, llvm::Value*> constantsBwd;

    SimulatedPtr unmeaningfulPtr;

    SimulatedPtr allocStart;
    SimulatedPtr allocEnd;

    SimulatedPtr mallocStart;
    SimulatedPtr mallocEnd;

    SimulatedPtr constantStart;
    SimulatedPtr constantEnd;

    SimulatedPtrSize currentAllocOffset;
    SimulatedPtrSize currentMallocOffset;
    SimulatedPtrSize currentConstantOffset;

    globalMemoryTable globals;
    const llvm::DataLayout* DL;

    Impl(const llvm::DataLayout& dl): DL(&dl) {
        TRACE_FUNC;

        auto grain = dl.getPointerABIAlignment();
        tree.chunk_size = K.get(1) * grain;
        tree.start = 1 << 20;
        tree.end = tree.start + (1ULL << M.get(33ULL)) * tree.chunk_size;

        allocStart = tree.start;
        allocEnd = (tree.end - tree.start) / 2 + tree.start;

        mallocStart = allocEnd;
        mallocEnd = tree.end;

        constantStart = 1 << 10;
        constantEnd = 1 << 20;

        unmeaningfulPtr = 1 << 8;

        currentAllocOffset = 0U;
        currentMallocOffset = 0U;
        currentConstantOffset = 0U;
    }

    void* getPointerToConstant(llvm::Value* v, size_t size) {
        TRACE_FUNC;
        if(constants.count(v)) return ptr_cast(constants.at(v));
        else {
            auto realPtr = currentConstantOffset + constantStart;
            currentConstantOffset += size;
            constants[v] = realPtr;
            constantsBwd[realPtr] = v;
            return ptr_cast(realPtr);
        }
    }

    void* getPointerToGlobalMemory(llvm::GlobalValue* gv, SimulatedPtrSize size, SimulatedPtrSize offset) {
        // FIXME: check offset for validity and throw an exception

        if (auto F = const_cast<llvm::Function*>(llvm::dyn_cast<llvm::Function>(gv))) {
            return getPointerToConstant(F, size);
        }

        if (auto&& pp = util::at(globals, llvm::dyn_cast<llvm::GlobalVariable>(gv)))
          return pp.getUnsafe().get() + offset;

        // Global variable might have been added since interpreter started.
        if (llvm::GlobalVariable *GVar =
                const_cast<llvm::GlobalVariable *>(llvm::dyn_cast<llvm::GlobalVariable>(gv))) {
            globals[GVar] = allocateMemoryForGV(GVar, *DL);
        } else UNREACHABLE("Global hasn't had an address allocated yet!");

        return util::at(globals, llvm::dyn_cast<llvm::GlobalVariable>(gv)).get() + offset;

    }

    ~Impl(){}
};

uintptr_t MemorySimulator::getQuant() const {
    return pimpl_->tree.chunk_size;
}

void* MemorySimulator::getOpaquePtr() {
    TRACE_FUNC;
    return ptr_cast(pimpl_->unmeaningfulPtr);
}

bool MemorySimulator::isOpaquePointer(void* ptr) {
    TRACE_FUNC;
    return pimpl_->unmeaningfulPtr == ptr_cast(ptr);
}

void* MemorySimulator::getPointerToFunction(llvm::Function* f, size_t size) {
    TRACE_FUNC;
    return pimpl_->getPointerToConstant(f, size);
}
void* MemorySimulator::getPointerBasicBlock(llvm::BasicBlock* bb, size_t size) {
    TRACE_FUNC;
    return pimpl_->getPointerToConstant(bb, size);
}
void* MemorySimulator::getPointerToGlobal(llvm::GlobalValue* gv, size_t size, SimulatedPtrSize offset) {
    TRACE_FUNC;

    return static_cast<uint8_t*>(pimpl_->getPointerToConstant(gv, size)) + offset;
}

llvm::Function* MemorySimulator::accessFunction(void* p) {
    TRACE_FUNC;
    auto realPtr = ptr_cast(p);
    if(realPtr > pimpl_->constantEnd || realPtr < pimpl_->constantStart) signalIllegalLoad(realPtr);
    auto result = util::at(pimpl_->constantsBwd, realPtr);
    if(!result) signalIllegalLoad(realPtr);

    return llvm::dyn_cast<llvm::Function>(result.getUnsafe());
}
llvm::BasicBlock* MemorySimulator::accessBasicBlock(void* p) {
    TRACE_FUNC;
    auto realPtr = ptr_cast(p);
    if(realPtr > pimpl_->constantEnd || realPtr < pimpl_->constantStart) signalIllegalLoad(realPtr);
    auto result = util::at(pimpl_->constantsBwd, realPtr);
    if(!result) signalIllegalLoad(realPtr);

    return llvm::dyn_cast<llvm::BasicBlock>(result.getUnsafe());
}
std::pair<llvm::GlobalValue*, SimulatedPtrSize> MemorySimulator::accessGlobal(void* p) {
    TRACE_FUNC;
    auto realPtr = ptr_cast(p);
    if(realPtr > pimpl_->constantEnd || realPtr < pimpl_->constantStart) signalIllegalLoad(realPtr);
    auto lb = pimpl_->constantsBwd.lower_bound(realPtr);

    if(lb == pimpl_->constantsBwd.end()) signalIllegalLoad(realPtr);

    return { llvm::dyn_cast<llvm::GlobalValue>(lb->second), realPtr - lb->first };
}

static SimulatedPtrSize calc_real_memory_amount(SimulatedPtrSize amount, SimulatedPtrSize chunk_size) {
    TRACE_FUNC;
    while(amount > chunk_size) {
        chunk_size <<= 1;
    }
    return chunk_size;
}

void* MemorySimulator::AllocateMemory(SimulatedPtrSize amount) {
    TRACE_FUNC;
    const auto real_amount = calc_real_memory_amount(amount, pimpl_->tree.chunk_size);

    SimulatedPtr ptr;
    if(pimpl_->currentAllocOffset % real_amount == 0) {
        ptr = pimpl_->allocStart + pimpl_->currentAllocOffset;
    } else {
        ptr = pimpl_->allocStart + (pimpl_->currentAllocOffset / real_amount + 1) * real_amount;
    }

    pimpl_->tree.allocate(ptr, amount, SegmentNode::MemoryState::Uninit, SegmentNode::MemoryStatus::Alloca);

    pimpl_->currentAllocOffset = ptr + real_amount - pimpl_->allocStart;

    return ptr_cast(ptr);
}

void MemorySimulator::DeallocateMemory(void* ptr) {
    TRACE_FUNC;

    auto realPtr = ptr_cast(ptr);
    pimpl_->tree.free(realPtr, SegmentNode::MemoryStatus::Alloca);
}

void* MemorySimulator::MallocMemory(SimulatedPtrSize amount, MallocFill fillWith) {
    TRACE_FUNC;
    const auto real_amount = calc_real_memory_amount(amount, pimpl_->tree.chunk_size);
    const auto memState = (fillWith == MallocFill::ZERO) ? SegmentNode::MemoryState::Memset : SegmentNode::MemoryState::Uninit;

    SimulatedPtr ptr;
    if(pimpl_->currentMallocOffset % real_amount == 0) {
        ptr = pimpl_->mallocStart + pimpl_->currentMallocOffset;
    } else {
        ptr = pimpl_->mallocStart + (pimpl_->currentMallocOffset / real_amount + 1) * real_amount;
    }

    pimpl_->tree.allocate(ptr, amount, memState, SegmentNode::MemoryStatus::Malloc);
    pimpl_->currentMallocOffset = ptr + real_amount - pimpl_->mallocStart;

    return ptr_cast(ptr);
}

void MemorySimulator::FreeMemory(void* ptr) {
    TRACE_FUNC;

    auto realPtr = ptr_cast(ptr);

    pimpl_->tree.free(realPtr, SegmentNode::MemoryStatus::Malloc);
}

static void assign(MemorySimulator::mutable_buffer_t dst, MemorySimulator::buffer_t src) {
    TRACE_FUNC;
    ASSERTC(dst.size() == src.size());
    std::copy(std::begin(src), std::end(src), std::begin(dst));
}

auto MemorySimulator::LoadBytesFromMemory(mutable_buffer_t buffer, buffer_t where) -> ValueState {
    TRACE_FUNC;
    ASSERTC(buffer.size() == where.size());

    const auto size = where.size();
    const auto chunk_size = pimpl_->tree.chunk_size;
    auto ptr = ptr_cast(where.data());
    auto offset = ptr - pimpl_->tree.start;
    auto loaded = (SimulatedPtrSize)0;

    if(ptr >= pimpl_->constantStart && ptr <= pimpl_->constantEnd) {
        auto&& gv = accessGlobal(const_cast<void*>(static_cast<const void*>(where.data())));
        auto ptr = pimpl_->getPointerToGlobalMemory(gv.first, size, gv.second);
        std::memcpy(buffer.data(), ptr, size);
        return ValueState::CONCRETE;
    }

    while(loaded < size) {
        const auto current = pimpl_->tree.get(ptr);

        auto current_size = chunk_size;
        // if we start in the middle of a chunk
        current_size -= offset % chunk_size;
        // if we end in the middle of a chunk
        if(size - loaded < chunk_size) {
            auto leftover = size - loaded - offset%chunk_size;
            current_size -= (chunk_size - leftover);
        }

        auto slice = buffer.slice(loaded, current_size);
        if(current.memState == SegmentNode::MemoryState::Memset) {
            std::memset(slice.data(), current.filledWith, slice.size());
        } else if(current.memState == SegmentNode::MemoryState::Uninit) {
            return ValueState::UNKNOWN;
        } else {
            assign(slice, buffer_t{current.data, current_size});
        }

        loaded += current_size;
        offset += current_size;
        ptr += current_size;
    }

    return ValueState::CONCRETE;
    // XXX: what about endianness?
}
void MemorySimulator::StoreBytesToMemory(buffer_t buffer, mutable_buffer_t where) {
    TRACE_FUNC;
    ASSERTC(buffer.size() == where.size());
    const auto Ptr = where.data();
    const auto size = where.size();

    const auto Src = buffer.data();
    const auto realPtr = ptr_cast(Ptr);

    pimpl_->tree.store(realPtr, Src, size);
    // XXX: what about endianness?
}

void MemorySimulator::StoreIntToMemory(const llvm::APInt& IntVal, mutable_buffer_t where) {
    TRACE_FUNC;
    const auto Ptr = where.data();
    auto size = where.size();
    const uint8_t* Src = (const uint8_t*) IntVal.getRawData();
    const auto realPtr = ptr_cast(Ptr);

    if(llvm::sys::IsLittleEndianHost) {
        // Little-endian host - the source is ordered from LSB to MSB
        // Order the destination from LSB to MSB
        // => Do a straight copy
        pimpl_->tree.store(realPtr, Src, size);
    } else {
        // Big-endian host - the source is an array of 64 bit words ordered from LSW to MSW
        // Each word is ordered from MSB to LSB
        // Order the destination from MSB to LSB
        // => Reverse the word order, but not the bytes in a word
        while(size > sizeof(uint64_t)) {
            size -= sizeof(uint64_t);
            // May not be aligned so use memcpy
            pimpl_->tree.store(realPtr + size, Src, sizeof(uint64_t));
            Src += sizeof(uint64_t);
        }

        pimpl_->tree.store(realPtr, Src + sizeof(uint64_t) - size, size);
    }
}

auto MemorySimulator::LoadIntFromMemory(llvm::APInt& val, buffer_t where) -> ValueState {
    TRACE_FUNC;
    const auto Ptr = where.data();
    auto size = where.size();
    const auto realPtr = ptr_cast(Ptr);
    const auto chunk_size = pimpl_->tree.chunk_size;

    ASSERTC(size <= chunk_size);

    // TODO: belyaev Think!
    if(realPtr / chunk_size != (realPtr + size -1) / chunk_size) {
        // if the piece crosses the chunk border
        TRACES() << "While loading from " << tfm::format("0x%x", realPtr) << endl;
        TRACES() << "Chunk size = " << chunk_size << endl;
        throw std::runtime_error("unsupported, sorry");
    }

    auto&& load = pimpl_->tree.get(realPtr);
    uint8_t* Dst = reinterpret_cast<uint8_t*>(const_cast<uint64_t*>(val.getRawData()));
    uint8_t* Src = load.data;

    if(load.memState == SegmentNode::MemoryState::Memset) {
        std::memset(Dst, load.filledWith , size);
        return ValueState::CONCRETE;
    } else if(load.memState == SegmentNode::MemoryState::Uninit) {
        return ValueState::UNKNOWN; // FIXME: implement uninit handling policy
    }

    if(llvm::sys::IsLittleEndianHost)
        // Little-endian host - the source is ordered from LSB to MSB
        // Order the destination from LSB to MSB
        // => Do a straight copy
        std::memcpy(Dst, Src, size);
    else {
        // Big-endian host - the source is an array of 64 bit words ordered from LSW to MSW
        // Each word is ordered from MSB to LSB
        // Order the destination from MSB to LSB
        // => Reverse the word order, but not the bytes in a word
        while(size > sizeof(uint64_t)) {
            size -= sizeof(uint64_t);
            // May not be aligned so use memcpy
            std::memcpy(Dst, Src + size, sizeof(uint64_t));
            Dst += sizeof(uint64_t);
        }

        std::memcpy(Dst + sizeof(uint64_t) - size, Src, size);
    }

    TRACES() << "Loaded value " << val.getLimitedValue() << endl;
    return ValueState::CONCRETE;
}

void MemorySimulator::Memset(void* dst, uint8_t fill, size_t size) {
    pimpl_->tree.memset(ptr_cast(dst), fill, size);
}

MemorySimulator::MemorySimulator(const llvm::DataLayout& dl) : pimpl_{new Impl{dl}} {}

MemorySimulator::~MemorySimulator() {}

} /* namespace borealis */

#include "Util/unmacros.h"
