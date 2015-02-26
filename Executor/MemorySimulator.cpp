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

#include <tinyformat/tinyformat.h>

#include "Executor/MemorySimulator.h"
#include "Executor/Exceptions.h"
#include "Util/util.h"
#include "Config/config.h"

#include "Logging/tracer.hpp"

#include "Util/macros.h"

namespace borealis {

using ChunkType = std::unique_ptr<uint8_t[]>;

using SimulatedPtr = std::uintptr_t;
using SimulatedPtrSize = std::uintptr_t;

config::ConfigEntry<int> K{"executor", "memory-chunk-multiplier"};
config::ConfigEntry<int> M{"executor", "memory-power"};

inline SimulatedPtr ptr_cast(const void* vd) {
    TRACE_FUNC;
    return reinterpret_cast<SimulatedPtr>(vd);
}

inline SimulatedPtr ptr_cast(const uint8_t* vd) {
    TRACE_FUNC;
    return reinterpret_cast<SimulatedPtr>(vd);
}

inline void* ptr_cast(SimulatedPtr vd) {
    TRACE_FUNC;
    return reinterpret_cast<void*>(vd);
}

struct SegmentNode {
    using Ptr = std::unique_ptr<SegmentNode>;
    enum class MemoryStatus{ Unallocated, Malloc, Alloca };
    enum class MemoryState{ Memset, Uninit, Unknown };

    MemoryStatus status = MemoryStatus::Unallocated;
    MemoryState state = MemoryState::Unknown;
    uint8_t memSetTo = 255;

    ChunkType chunk = nullptr;

    Ptr left = nullptr;
    Ptr right = nullptr;

    SimulatedPtrSize reallyAllocated = 0U;
};

static const char* printStatus( SegmentNode::MemoryStatus st ) {
    switch(st) {
    case SegmentNode::MemoryStatus::Unallocated: return "Unallocated";
    case SegmentNode::MemoryStatus::Malloc: return "Malloc";
    case SegmentNode::MemoryStatus::Alloca: return "Alloca";
    }
}

static const char* printState( SegmentNode::MemoryState st ) {
    switch(st) {
    case SegmentNode::MemoryState::Memset: return "Memset";
    case SegmentNode::MemoryState::Uninit: return "Uninit";
    case SegmentNode::MemoryState::Unknown: return "Unknown";
    }
}

static SimulatedPtr middle(SimulatedPtr from, SimulatedPtr to) {
    TRACE_FUNC;
    return from + (to - from)/2;
}

static void signalUnsupported(SimulatedPtr where) {
    TRACE_FUNC;
    throw std::runtime_error("Unsupported operation on " + tfm::format("0x%x", where));
}

static void signalIllegalFree(SimulatedPtr where) {
    TRACE_FUNC;
    throw illegal_mem_free_exception(ptr_cast(where));
}

static void signalIllegalLoad(SimulatedPtr where) {
    TRACE_FUNC;
    throw illegal_mem_read_exception(ptr_cast(where));
}

static void signalIllegalStore(SimulatedPtr where) {
    TRACE_FUNC;
    throw illegal_mem_write_exception(ptr_cast(where));
}

static void signalInconsistency(const std::string& error) {
    TRACE_FUNC;
    throw std::logic_error(error);
}

static void signalOutOfMemory(SimulatedPtrSize amount) {
    TRACE_FUNC;
    throw out_of_memory_exception{};
}

static SegmentNode::Ptr& force(SegmentNode::Ptr& t) {
    TRACE_FUNC;
    if(t == nullptr) t.reset(new SegmentNode{});
    return t;
}

struct emptyTraverser {
    void handleGoLeft(SegmentNode&) {}
    void handleGoRight(SegmentNode&) {}
    void handleEmptyNode(SimulatedPtrSize) {}
};

struct stateInvalidatingTraverser: emptyTraverser {
    void forceChildrenAndDeriveState(SegmentNode& t){
        if(!t.left) {
            force(t.left)->state = t.state;
            t.left->memSetTo = t.memSetTo;
        }
        if(!t.right) {
            force(t.right)->state = t.state;
            t.right->memSetTo = t.memSetTo;
        }
        t.state = SegmentNode::MemoryState::Unknown;
    }

    void handleGoLeft(SegmentNode& t) {
        TRACE_FUNC;
        TRACES() << printState(t.state) << endl;
        forceChildrenAndDeriveState(t);
    }

    void handleGoRight(SegmentNode& t) {
        TRACE_FUNC;
        TRACES() << printState(t.state) << endl;
        forceChildrenAndDeriveState(t);
    }
};

struct SegmentTree {
    SimulatedPtr start;
    SimulatedPtr end;
    SimulatedPtrSize chunk_size;
    SegmentNode::Ptr root = nullptr;


    template<class Traverser>
    void traverse(SimulatedPtr where, Traverser& theTraverser) {
        TRACE_FUNC;
        return traverse<Traverser>(where, theTraverser, root, start, end);
    }

    template<class Traverser>
    void traverse(SimulatedPtr where, Traverser&& theTraverser) {
        TRACE_FUNC;
        auto tmpTraverser = std::move(theTraverser);
        return traverse<Traverser>(where, tmpTraverser, root, start, end);
    }

    template<class Traverser>
    void traverse(
            SimulatedPtr where,
            Traverser& theTraverser,
            SegmentNode::Ptr& t,
            SimulatedPtr minbound,
            SimulatedPtr maxbound) {
        TRACE_FUNC;

        if(!t) theTraverser.handleEmptyNode(where);

        auto mid = middle(minbound, maxbound);

        force(t);
        if(theTraverser.handlePath(this, minbound, maxbound, t, where)) {
            return;
        } else if(where >= minbound && where < mid) {
            theTraverser.handleGoLeft(*t);
            traverse<Traverser>( where, theTraverser, t->left, minbound, mid);
        } else if(where >= mid && where < maxbound) {
            theTraverser.handleGoRight(*t);
            traverse<Traverser>(where, theTraverser, t->right, mid, maxbound);
        } else {
            signalInconsistency(__PRETTY_FUNCTION__);
        }
    }

    inline void allocate(
            SimulatedPtr where,
            SimulatedPtrSize size,
            SegmentNode::MemoryState state,
            SegmentNode::MemoryStatus status) {
        TRACE_FUNC;
        if(size > (end - start)) signalOutOfMemory(size);

        struct allocator: stateInvalidatingTraverser {
            SegmentNode::MemoryState state;
            SegmentNode::MemoryStatus status;
            SimulatedPtrSize size;

            allocator(
                SegmentNode::MemoryState state,
                SegmentNode::MemoryStatus status,
                SimulatedPtrSize size
            ): state(state), status(status), size(size) {};

            bool handlePath(
                    SegmentTree* tree,
                    SimulatedPtrSize minbound,
                    SimulatedPtrSize maxbound,
                    SegmentNode::Ptr& t,
                    SimulatedPtrSize where) {
                TRACE_FUNC;
                auto available = maxbound - minbound;

                TRACES() << "Node " << ((void*)t.get()) << endl;
                TRACES() << "Left " << ((void*)t->left.get()) << endl;
                TRACES() << "Right " << ((void*)t->right.get()) << endl;
                TRACES() << '{' << tfm::format("0x%x", minbound) << ','
                                << tfm::format("0x%x", maxbound) << '}' << endl;
                TRACES() << "Where: " << tfm::format("0x%x", where) << endl;
                TRACES() << "Size: " << size << endl;

                if(minbound == where
                    &&
                        ((available/2ULL < size && available >= size)
                       ||(available == tree->chunk_size && size <= tree->chunk_size))) {
                    t->reallyAllocated = size;
                    t->state = state;
                    if(state == SegmentNode::MemoryState::Memset) t->memSetTo = 0;
                    t->status = status;
                    TRACES() << "Allocated segment {" << tfm::format("0x%x", minbound)
                            << "," << tfm::format("0x%x", minbound + size) << "}" << endl;
                    TRACES() << "Node " << ((void*)t.get()) << endl;
                    TRACES() << (t->status != SegmentNode::MemoryStatus::Unallocated) << endl;
                    return true;
                }

                return false;
            }
        };

        allocator alloc{state, status, size};
        traverse(where, alloc);
    }

    inline void store(
            SimulatedPtr where,
            const uint8_t* data,
            SimulatedPtrSize size) {
        TRACE_FUNC;

        struct storeTraverser: stateInvalidatingTraverser {
            const uint8_t* data;
            SimulatedPtrSize size;
            bool didAlloc = false;
            storeTraverser(const uint8_t* data, SimulatedPtrSize size, bool didAlloc)
                :data(data), size(size), didAlloc(didAlloc) {}

            bool handlePath(SegmentTree* tree,
                SimulatedPtrSize minbound,
                SimulatedPtrSize maxbound,
                SegmentNode::Ptr& t,
                SimulatedPtrSize where) {
                TRACE_FUNC;


                auto available = maxbound - minbound;
                auto mid = middle(minbound, maxbound);

                TRACES() << "where: " << tfm::format("0x%x", where) << endl;
                TRACES() << "size: " << size << endl;
                TRACES() << "available: " << available << endl;
                TRACES() << '{' << tfm::format("0x%x", minbound) << ','
                                << tfm::format("0x%x", maxbound) << ','
                                << didAlloc << ','
                                << printStatus(t->status) << ','
                                << printState(t->state) << '}' << endl;

                if(t->status != SegmentNode::MemoryStatus::Unallocated) {
                    if(didAlloc) {
                        signalInconsistency("Allocated segment inside other allocated segment detected");
                    }
                    didAlloc = true;
                    TRACES() << "Found allocated segment!" << endl;
                    TRACES() << "{" << tfm::format("0x%x", minbound) << ","
                             << tfm::format("0x%x", minbound + t->reallyAllocated) << "}" << endl;

                    if(where >= minbound + t->reallyAllocated) {
                        signalIllegalStore(where);
                    }
                }

                if(available == tree->chunk_size && size <= tree->chunk_size) {
                    TRACES() << "Storing!" << endl;

                    t->state = SegmentNode::MemoryState::Unknown;

                    if(!didAlloc) signalIllegalStore(where);
                    if(!t->chunk) t->chunk.reset(new unsigned char[tree->chunk_size]);
                    std::memcpy(t->chunk.get() + (where - minbound), data, size);
                } else if(where < mid && where + size > mid) {
                    auto leftChunkSize = mid - where;
                    auto rightChunkSize = size - leftChunkSize;

                    forceChildrenAndDeriveState(*t);

                    storeTraverser left{ data, leftChunkSize, didAlloc };
                    tree->traverse(where, left, t->left, minbound, mid);
                    storeTraverser right{ data + leftChunkSize, rightChunkSize, didAlloc };
                    tree->traverse(where, right, t->right, mid, maxbound);
                } else return false;
                return true;
            }
        };

        storeTraverser traverser{ data, size, false };
        return traverse(where, traverser);
    }

    struct intervalState {
        uint8_t* data;
        SegmentNode::MemoryState memState;
        uint8_t filledWith;
    };

    inline intervalState get(SimulatedPtr where) {
        TRACE_FUNC;
        struct loader: public emptyTraverser {
            uint8_t* ptr = nullptr;
            SegmentNode::MemoryState state = SegmentNode::MemoryState::Unknown;
            uint8_t filledWith = 0xFF;

            void handleEmptyNode(SimulatedPtrSize where) {
                TRACE_FUNC;
                signalIllegalLoad(where);
            }

            bool handlePath(SegmentTree* tree,
                SimulatedPtrSize minbound,
                SimulatedPtrSize maxbound,
                SegmentNode::Ptr& t,
                SimulatedPtrSize where) {
                TRACE_FUNC;

                TRACES() << "where: " << tfm::format("0x%x", where) << endl;

                auto available = maxbound - minbound;

                TRACES() << "available: " << available << endl;
                TRACES() << '{' << tfm::format("0x%x", minbound) << ',' << tfm::format("0x%x", maxbound)
                         << ',' << printStatus(t->status) << ',' << printState(t->state) << '}' << endl;

                if(t->state != SegmentNode::MemoryState::Unknown) {
                    state = t->state;
                    filledWith = t->memSetTo;
                    return true;
                }

                if(available == tree->chunk_size) {
                    auto offset = where - minbound;
                    ptr = t->chunk.get() + offset;
                    return true;
                }

                return false;
            }
        } loadTraverser;

        traverse(where, loadTraverser);

        return { loadTraverser.ptr, loadTraverser.state, loadTraverser.filledWith };
    }

    inline void free(SimulatedPtr where, SegmentNode::MemoryStatus desiredStatus) {
        TRACE_FUNC;

        if(where < start || where >= end) signalIllegalFree(where);

        struct liberator : emptyTraverser {
            SegmentNode::MemoryStatus desiredStatus;

            liberator(SegmentNode::MemoryStatus desiredStatus): desiredStatus{desiredStatus} {}

            void handleEmptyNode(SimulatedPtr where) {
                TRACE_FUNC;
                signalIllegalFree(where);
            }

            bool handlePath(SegmentTree*,
                SimulatedPtrSize minbound,
                SimulatedPtrSize,
                SegmentNode::Ptr& t,
                SimulatedPtrSize where){
                TRACE_FUNC;
                if(minbound == where && t->status == desiredStatus) {
                    t.reset();
                    return true;
                }
                return false;
            }
        } liberator{desiredStatus};

        return traverse(where, liberator);
    }

    inline void memset(SimulatedPtr where, uint8_t fill, SimulatedPtrSize size) {
        TRACE_FUNC;

        if(where < start || where >= end) signalIllegalStore(where);

        struct memsetter: stateInvalidatingTraverser {
            SimulatedPtr dest;
            uint8_t fill;
            SimulatedPtrSize size;
            memsetter(SimulatedPtr dest, uint8_t fill, SimulatedPtrSize size):
                dest(dest), fill(fill), size(size) {};

            bool handlePath(SegmentTree* tree,
                SimulatedPtrSize minbound,
                SimulatedPtrSize maxbound,
                SegmentNode::Ptr& t,
                SimulatedPtrSize where) {
                TRACE_FUNC;

                TRACES() << "where: " << tfm::format("0x%x", where) << endl;

                auto available = maxbound - minbound;
                auto mid = middle(minbound, maxbound);

                TRACES() << "available: " << available << endl;
                TRACES() << '{' << tfm::format("0x%x", minbound) << ',' << tfm::format("0x%x", maxbound)
                         << ',' << printStatus(t->status) << ',' << printState(t->state) << '}' << endl;

                if(t->state != SegmentNode::MemoryState::Unknown
                   && (minbound <= dest)
                   && (maxbound >= dest + size)) {
                    t->state = SegmentNode::MemoryState::Memset;
                    t->memSetTo = fill;
                    return true;
                } else if(available == tree->chunk_size && size == tree->chunk_size) {
                    t->state = SegmentNode::MemoryState::Memset;
                    t->memSetTo = fill;
                    return true;
                } else if(available == tree->chunk_size && size <= tree->chunk_size) {
                    ASSERTC(where >= minbound);

                    auto offset = where - minbound;
                    ASSERTC(offset + size < maxbound);

                    auto realDst = t->chunk.get() + offset;
                    std::memset(realDst, fill, size);

                    return true;
                } else if(where < mid && where + size > mid) {
                    auto leftChunkSize = mid - where;
                    auto rightChunkSize = size - leftChunkSize;

                    forceChildrenAndDeriveState(*t);

                    memsetter left{ dest, fill, leftChunkSize };
                    tree->traverse(where, left, t->left, minbound, mid);
                    memsetter right{ dest + leftChunkSize, fill, rightChunkSize };
                    tree->traverse(where, right, t->right, mid, maxbound);
                } else return false;
                return true;
            }
        } memsetter{ where, fill, size };

        return traverse(where, memsetter);

    }

    inline void memmove(SimulatedPtr src, SimulatedPtr dst, SimulatedPtrSize sz) {

        struct intervalInfo {
            uint8_t* data;
            SegmentNode::MemoryState memState;
            uint8_t filledWith;

            SimulatedPtr start;
            SimulatedPtr end;

            intervalInfo(uint8_t *data, SegmentNode::MemoryState memState, uint8_t filledWith, SimulatedPtr start, SimulatedPtr end)
                    : data(data), memState(memState), filledWith(filledWith), start(start), end(end) {
            }
        };

        struct intervalCollector: emptyTraverser {
            SimulatedPtr src;
            SimulatedPtrSize sz;
            std::vector<intervalInfo>* path;

            intervalCollector(SimulatedPtr src, SimulatedPtrSize sz, std::vector<intervalInfo> *path)
                    : src(src), sz(sz), path(path) {
            }

            void handleEmptyNode(SimulatedPtrSize where) {
                TRACE_FUNC;
                signalIllegalLoad(where);
            }

            bool handlePath(
                SegmentTree* tree,
                SimulatedPtrSize minbound,
                SimulatedPtrSize maxbound,
                SegmentNode::Ptr& t,
                SimulatedPtrSize where
            ) {
                TRACE_FUNC;
                TRACES() << "where: " << tfm::format("0x%x", where) << endl;

                auto available = maxbound - minbound;
                auto mid = middle(minbound, maxbound);

                TRACES() << "available: " << available << endl;
                TRACES() << '{' << tfm::format("0x%x", minbound) << ',' << tfm::format("0x%x", maxbound)
                         << ',' << printStatus(t->status) << ',' << printState(t->state) << '}' << endl;

                if(t->state != SegmentNode::MemoryState::Unknown) {
                    path->emplace_back(
                        nullptr,
                        t->state,
                        t->memSetTo,
                        std::max(minbound, where),
                        std::min(maxbound, where + sz)
                    );
                } else if(available == tree->chunk_size) {
                    auto startOffset = where - minbound;

                    path->emplace_back(
                        t->chunk.get() + startOffset,
                        SegmentNode::MemoryState::Unknown,
                        0,
                        std::max(minbound, where),
                        std::min(maxbound, where + sz)
                    );
                } else if(where < mid && (where + sz) > mid) {
                    auto leftChunkSize = mid - where;
                    auto rightChunkSize = sz - leftChunkSize;

                    intervalCollector left{ src, leftChunkSize, path };
                    tree->traverse(where, left, t->left, minbound, mid);
                    intervalCollector right{ src + leftChunkSize, rightChunkSize, path };
                    tree->traverse(where, right, t->right, mid, maxbound);
                } else return false;

                return true;
            }
        };

        std::vector<intervalInfo> sourceInterval;
        std::vector<intervalInfo> destinationInterval;

        traverse(src, intervalCollector{ src, sz, &sourceInterval });
        traverse(src, intervalCollector{ dst, sz, &destinationInterval });



    }

};

struct MemorySimulator::Impl {
    SegmentTree tree;
    std::unordered_map<llvm::Value*, SimulatedPtr> constants;
    std::unordered_map<SimulatedPtr, llvm::Value*> constantsBwd;

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

    Impl(SimulatedPtrSize grain) {
        TRACE_FUNC;
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
void* MemorySimulator::getPointerToGlobal(llvm::GlobalValue* gv, size_t size) {
    TRACE_FUNC;
    return pimpl_->getPointerToConstant(gv, size);
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
llvm::GlobalValue* MemorySimulator::accessGlobal(void* p) {
    TRACE_FUNC;
    auto realPtr = ptr_cast(p);
    if(realPtr > pimpl_->constantEnd || realPtr < pimpl_->constantStart) signalIllegalLoad(realPtr);
    auto result = util::at(pimpl_->constantsBwd, realPtr);
    if(!result) signalIllegalLoad(realPtr);

    return llvm::dyn_cast<llvm::GlobalValue>(result.getUnsafe());
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
    ASSERTC(buffer.size() == where.size());
    const auto size = where.size();
    const auto chunk_size = pimpl_->tree.chunk_size;
    auto ptr = ptr_cast(where.data());
    auto offset = ptr - pimpl_->tree.start;
    auto loaded = (SimulatedPtrSize)0;

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


MemorySimulator::MemorySimulator(SimulatedPtrSize grain) : pimpl_{new Impl{grain}} {}

MemorySimulator::~MemorySimulator() {}

} /* namespace borealis */

#include "Util/unmacros.h"
