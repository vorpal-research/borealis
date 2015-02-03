/*
 * MemorySimulator.cpp
 *
 *  Created on: Jan 28, 2015
 *      Author: belyaev
 */

#include <llvm/Support/Host.h>

#include <cstdint>
#include <cstring>

#include "Executor/MemorySimulator.h"
#include "Util/util.h"

namespace borealis {

using ChunkType = std::unique_ptr<unsigned char[]>;

using SimulatedPtr = std::uintptr_t;
using SimulatedPtrSize = std::uintptr_t;

struct SegmentNode {
    using Ptr = std::unique_ptr<SegmentNode>;
    enum class MemoryStatus{ Unallocated, Malloc, Alloca };
    enum class MemoryState{ Zero, Uninit, Unknown };

    MemoryStatus status = MemoryStatus::Unallocated;
    MemoryState state = MemoryState::Unknown;

    ChunkType chunk = nullptr;

    Ptr left = nullptr;
    Ptr right = nullptr;

    SimulatedPtrSize reallyAllocated = 0U;
};

struct SegmentTree {
    SimulatedPtr start;
    SimulatedPtr end;
    SimulatedPtrSize chunk_size;
    SegmentNode::Ptr root = nullptr;

    static SegmentNode::Ptr& force(SegmentNode::Ptr& t) {
        if(t == nullptr) t.reset(new SegmentNode{});
        return t;
    }

    static SimulatedPtr middle(SimulatedPtr from, SimulatedPtr to) {
        return from + (to - from)/2;
    }

    void signalUnsupported(SimulatedPtr where) {
        throw new std::runtime_error("Unsupported operation on " + util::toString(where));
    }

    void signalIllegalFree(SimulatedPtr where) {
        throw new std::runtime_error("Illegal free() call at " + util::toString(where));
    }

    void signalIllegalLoad(SimulatedPtr where) {
        throw new std::runtime_error("Memory read violation at " + util::toString(where));
    }

    void signalIllegalStore(SimulatedPtr where) {
        throw new std::runtime_error("Memory write violation at " + util::toString(where));
    }

    void signalInconsistency(const std::string& error) {
        throw new std::logic_error(error);
    }


    inline SegmentNode::Ptr allocate(
            SimulatedPtr where,
            SimulatedPtrSize size,
            SegmentNode::MemoryState state,
            SegmentNode::MemoryStatus status) {
        return allocate(where, size, state, status, root, start, end);
    }
    SegmentNode::Ptr allocate(
            SimulatedPtr where,
            SimulatedPtrSize size,
            SegmentNode::MemoryState state,
            SegmentNode::MemoryStatus status,
            SegmentNode::Ptr& t,
            SimulatedPtr minbound,
            SimulatedPtr maxbound) {
        auto mid = middle(minbound, maxbound);
        auto available = maxbound - minbound;

        force(t);

        if(minbound == where && available/2 < size && available >= size) {
            t->reallyAllocated = size;
            t->state = state;
            t->status = status;
        } else if(where >= minbound && where < mid) {
            if(!t->right) force(t->right)->state = t->state;

            t->state = SegmentNode::MemoryState::Unknown;
            t->left = std::move(allocate(where, size, state, status, t->left, minbound, mid));
        } else if(where >= mid && where < maxbound) {
            if(!t->left) force(t->left)->state = t->state;

            t->state = SegmentNode::MemoryState::Unknown;
            t->right = std::move(allocate(where, size, state, status, t->right, mid, maxbound));
        } else {
            signalInconsistency(__PRETTY_FUNCTION__);
        }

        return std::move(t);
    }


    inline SegmentNode::Ptr store(
            SimulatedPtr where,
            const uint8_t* data,
            SimulatedPtrSize size) {
        return store(where, data, size, root, start, end);
    }
    SegmentNode::Ptr store(
            SimulatedPtr where,
            const uint8_t* data,
            SimulatedPtrSize size,
            SegmentNode::Ptr& t,
            SimulatedPtr minbound,
            SimulatedPtr maxbound,
            bool didAlloc = false) {
        auto mid = middle(minbound, maxbound);
        auto available = maxbound - minbound;

        force(t);

        if(t->status != SegmentNode::MemoryStatus::Unallocated) {
            if(didAlloc) {
                signalInconsistency("Allocated segment inside other allocated segment detected");
            }
            didAlloc = true;
            if(where > start + t->reallyAllocated) {
                signalIllegalStore(where);
            }
        }

        if(available == chunk_size && size <= chunk_size) {
            if(!didAlloc) signalIllegalStore(where);
            t->chunk.reset(new unsigned char[chunk_size]);
            std::memcpy(t->chunk.get(), data, size);
        } else if(where < mid && where + size >= mid) {
            auto leftChunkSize = mid - where;
            auto rightChunkSize = size - leftChunkSize;

            t->state = SegmentNode::MemoryState::Unknown;
            t->left = std::move(store(where, data, leftChunkSize, t->left, minbound, mid, didAlloc));
            t->right = std::move(store(mid, data + leftChunkSize, rightChunkSize, t->right, mid, maxbound, didAlloc));
        } else if(where >= minbound && where < mid) {
            if(!t->right) force(t->right)->state = t->state;

            t->state = SegmentNode::MemoryState::Unknown;
            t->left = std::move(store( where, data, size, t->left, minbound, mid, didAlloc));
        } else if(where >= mid && where < maxbound) {
            if(!t->left) force(t->left)->state = t->state;

            t->state = SegmentNode::MemoryState::Unknown;
            t->right = std::move(store(where, data, size, t->right, mid, maxbound, didAlloc));
        } else {
            signalInconsistency(__PRETTY_FUNCTION__);
        }

        return std::move(t);
    }


    inline std::pair<unsigned char*, SegmentNode::MemoryState> get(SimulatedPtr where) {
        return get(where, root, start, end);
    }
    std::pair<unsigned char*, SegmentNode::MemoryState> get(
            SimulatedPtr where,
            SegmentNode::Ptr& t,
            SimulatedPtr minbound,
            SimulatedPtr maxbound) {
        if(!t) signalIllegalLoad(where);

        auto mid = middle(minbound, maxbound);
        auto available = maxbound - minbound;

        force(t);

        if(t->state != SegmentNode::MemoryState::Unknown) return { nullptr, t->state };

        if(available == chunk_size) {
            auto offset = where - minbound;
            return { t->chunk.get() + offset, SegmentNode::MemoryState::Unknown };
        } else if(where >= minbound && where < mid) {
            return get(where, t->left, minbound, mid);
        } else if(where >= mid && where < maxbound) {
            return get(where, t->right, mid, maxbound);
        }

        signalIllegalLoad(where);
        return {};
    }


    inline void free(SimulatedPtr where) {
        return free(where, root, start, end);
    }
    void free(SimulatedPtr where,
            SegmentNode::Ptr& t,
            SimulatedPtr minbound,
            SimulatedPtr maxbound) {
        if(!t) signalIllegalFree(where);

        auto mid = middle(minbound, maxbound);
        auto available = maxbound - minbound;

        force(t);

        if(minbound == where && t->status == SegmentNode::MemoryStatus::Malloc) {
            t = nullptr;
        } else if(where >= minbound && where < mid) {
            return free(where, t->left, minbound, mid);
        } else if(where >= mid && where < maxbound) {
            return free(where, t->right, mid, maxbound);
        }

        signalIllegalFree(where);
    }
};

struct MemorySimulator::Impl {
    SegmentTree tree;

    SimulatedPtr allocStart;
    SimulatedPtr allocEnd;

    SimulatedPtr mallocStart;
    SimulatedPtr mallocEnd;

    SimulatedPtrSize currentAllocOffset;
    SimulatedPtrSize currentMallocOffset;
};

static SimulatedPtrSize calc_real_memory_amount(SimulatedPtrSize amount, SimulatedPtrSize chunk_size) {
    while(amount > chunk_size) {
        chunk_size <<= 1;
    }
    return chunk_size;
}

void* MemorySimulator::AllocateMemory(SimulatedPtrSize amount) {
    const auto chunk_size = pimpl_->tree.chunk_size;
    const auto real_amount = calc_real_memory_amount(amount, pimpl_->tree.chunk_size);

    SimulatedPtr ptr;
    if(pimpl_->currentAllocOffset % real_amount == 0) {
        ptr = pimpl_->allocStart + pimpl_->currentAllocOffset;
    } else {
        ptr = pimpl_->allocStart + (pimpl_->currentAllocOffset / real_amount + 1) * real_amount;
    }

    pimpl_->tree.allocate(ptr, amount, SegmentNode::MemoryState::Uninit, SegmentNode::MemoryStatus::Alloca);
    pimpl_->currentAllocOffset = ptr + real_amount - pimpl_->allocStart;

    return reinterpret_cast<void*>(ptr);
}

void* MemorySimulator::MallocMemory(SimulatedPtrSize amount, MallocFill fillWith) {
    const auto chunk_size = pimpl_->tree.chunk_size;
    const auto real_amount = calc_real_memory_amount(amount, pimpl_->tree.chunk_size);
    const auto memState = (fillWith == MallocFill::ZERO) ? SegmentNode::MemoryState::Zero : SegmentNode::MemoryState::Uninit;

    SimulatedPtr ptr;
    if(pimpl_->currentMallocOffset % real_amount == 0) {
        ptr = pimpl_->mallocStart + pimpl_->currentMallocOffset;
    } else {
        ptr = pimpl_->mallocStart + (pimpl_->currentMallocOffset / real_amount + 1) * real_amount;
    }

    pimpl_->tree.allocate(ptr, amount, memState, SegmentNode::MemoryStatus::Malloc);
    pimpl_->currentMallocOffset = ptr + real_amount - pimpl_->mallocStart;

    return reinterpret_cast<void*>(ptr);
}

void MemorySimulator::StoreIntToMemory(const llvm::APInt& IntVal, uint8_t* Ptr, size_t size) {
    const uint8_t* Src = (const uint8_t*) IntVal.getRawData();
    const auto realPtr = reinterpret_cast<SimulatedPtr>(Ptr);
    const auto chunkSize = pimpl_->tree.chunk_size;

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

void MemorySimulator::LoadIntFromMemory(llvm::APInt& val, uint8_t* Ptr, size_t size) {
    const auto realPtr = reinterpret_cast<SimulatedPtr>(Ptr);
    const auto chunkSize = pimpl_->tree.chunk_size;

    // TODO: belyaev Think!
    if(realPtr / chunkSize != (realPtr + size) / chunkSize) {
        // if the piece crosses the chunk border
        throw new std::runtime_error("unsupported, sorry");
    }

    if(size <= chunkSize) {
        auto&& load = pimpl_->tree.get(realPtr);
        if(load.second == SegmentNode::MemoryState::Zero) {
            return; // val is already zero
        } else if(load.second == SegmentNode::MemoryState::Uninit) {
            return; // FIXME: implement uninit handling policy
        }

        uint8_t* Dst = reinterpret_cast<uint8_t*>(const_cast<uint64_t*>(val.getRawData()));
        uint8_t* Src = load.first;

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
    }
}

MemorySimulator::MemorySimulator() : pimpl_{new Impl{}} {}

MemorySimulator::~MemorySimulator() {}

} /* namespace borealis */
