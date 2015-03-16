/*
 * SegmentTreeImpl.cpp
 *
 *  Created on: Mar 4, 2015
 *      Author: belyaev
 */

#include <memory>

#include <Executor/MemorySimulator/SegmentTreeImpl.h>

#include "Logging/tracer.hpp"

#include "Util/macros.h"

namespace borealis {

namespace {

struct allocTraverser: stateInvalidatingTraverser {
    const SegmentNode::MemoryState state;
    const SegmentNode::MemoryStatus status;
    const SimulatedPtrSize size;

    allocTraverser(
        SegmentNode::MemoryState state,
        SegmentNode::MemoryStatus status,
        SimulatedPtrSize size
    ): state(state), status(status), size(size) {
        TRACE_FUNC;
        TRACE_PARAM(state);
        TRACE_PARAM(status);
        TRACE_PARAM(size);
    };

    bool handleChunk(
            SegmentTree* tree,
            SimulatedPtrSize minbound,
            SimulatedPtrSize maxbound,
            SegmentNode::Ptr& t,
            SimulatedPtrSize /* where */
        ) {
        TRACE_FUNC;
        ASSERTC(size <= (maxbound - minbound));
        ASSERTC(size <= tree->chunk_size);

        t->reallyAllocated = size;
        t->state = state;
        if(state == SegmentNode::MemoryState::Memset) t->memSetTo = 0;
        t->status = status;

        TRACE_FMT("Allocated chunk segment { %s, %s }", minbound, minbound + size);
        return true;
    }

    bool handlePath(
            SegmentTree* /* tree */,
            SimulatedPtrSize minbound,
            SimulatedPtrSize maxbound,
            SegmentNode::Ptr& t,
            SimulatedPtrSize where) {
        TRACE_FUNC;
        auto available = maxbound - minbound;
        ASSERTC(size <= available);

        TRACE_PARAM(where);
        TRACE_PARAM(t.get());
        TRACE_PARAM(minbound);
        TRACE_PARAM(maxbound);
        TRACE_PARAM(size);

        if(minbound == where
            && (available/2ULL < size && available >= size)) {
            t->reallyAllocated = size;
            t->state = state;
            if(state == SegmentNode::MemoryState::Memset) t->memSetTo = 0;
            t->status = status;

            TRACE_FMT("Allocated segment { %s, %s }", minbound, minbound + size);
            TRACE_PARAM(t->status);
            return true;
        }

        return false;
    }
};

struct storeTraverser: stateInvalidatingTraverser {
    const uint8_t* const data;
    const SimulatedPtrSize size;

    bool didAlloc = false;

    storeTraverser(const uint8_t* data, SimulatedPtrSize size, bool didAlloc = false)
        :data(data), size(size), didAlloc(didAlloc) {
        TRACE_FUNC;
        TRACE_PARAM((void*)data);
        TRACE_PARAM(size);
        TRACE_PARAM(didAlloc);
    }

    void checkAllocation(SegmentNode::Ptr& t, SimulatedPtr minbound, SimulatedPtr where) {
        TRACE_FUNC;
        if(t->status != SegmentNode::MemoryStatus::Unallocated) {
            if(didAlloc) {
                signalInconsistency("Allocated segment inside other allocated segment detected");
            }
            didAlloc = true;
            TRACES() << "Found allocated segment:" << endl;
            TRACE_FMT("Allocated segment { %s, %s }", minbound, minbound + t->reallyAllocated);
            TRACE_PARAM(where);
            TRACE_PARAM(size);
            TRACE_PARAM(minbound);
            TRACE_PARAM(t->reallyAllocated);

            if(where + size >= minbound + t->reallyAllocated) {
                signalIllegalStore(where);
            }
        }
    }

    void handleChunk(SegmentTree* tree,
                    SimulatedPtrSize minbound,
                    SimulatedPtrSize /* maxbound */,
                    SegmentNode::Ptr& t,
                    SimulatedPtrSize where) {
        TRACE_FUNC;

        checkAllocation(t, minbound, where);

        ASSERTC(size <= tree->chunk_size);

        TRACES() << "Storing!" << endl;

        t->state = SegmentNode::MemoryState::Unknown;

        if(!didAlloc) signalIllegalStore(where);
        const auto offset = where - minbound;
        t->allocateChunk(tree->chunk_size);
        std::memcpy(t->chunk.get() + offset, data, size);
    }

    bool handlePath(SegmentTree* tree,
        SimulatedPtrSize minbound,
        SimulatedPtrSize maxbound,
        SegmentNode::Ptr& t,
        SimulatedPtrSize where) {
        TRACE_FUNC;

        auto available = maxbound - minbound;
        auto mid = middle(minbound, maxbound);

        TRACE_PARAM(where);
        TRACE_PARAM(size);
        TRACE_PARAM(available);
        TRACE_PARAM(minbound);
        TRACE_PARAM(maxbound);
        TRACE_PARAM(didAlloc);
        TRACE_PARAM(t->status);
        TRACE_PARAM(t->state);

        checkAllocation(t, minbound, where);

        if(where < mid && where + size > mid) {
            auto leftChunkSize = mid - where;
            auto rightChunkSize = size - leftChunkSize;

            forceChildrenAndDeriveState(*t);

            storeTraverser left{ data, leftChunkSize, didAlloc };
            tree->traverse(where, left, t->left, minbound, mid);
            storeTraverser right{ data + leftChunkSize, rightChunkSize, didAlloc };
            tree->traverse(mid, right, t->right, mid, maxbound);
            return true;
        }
        return false;
    }
};

struct loadTraverser: public emptyTraverser {
    std::stack<const SegmentNode*> backTrace;

    uint8_t* ptr = nullptr;
    size_t dataSize = 0;
    SegmentNode::MemoryState state = SegmentNode::MemoryState::Unknown;
    uint8_t filledWith = 0xFF;

    void handleEmptyNode(SimulatedPtrSize where) {
        TRACE_FUNC;
        signalIllegalLoad(where);
    }

    bool handleMemState(SegmentNode::Ptr& t) {
        TRACE_FUNC;
        if(t->state != SegmentNode::MemoryState::Unknown) {
            state = t->state;
            filledWith = t->memSetTo;
            return true;
        } else return false;
    }

    void handleChunk(SegmentTree* /* tree */,
        SimulatedPtrSize minbound,
        SimulatedPtrSize maxbound,
        SegmentNode::Ptr& t,
        SimulatedPtrSize where) {
        TRACE_FUNC;
        if(!handleMemState(t)) {
            auto offset = where - minbound;
            ptr = t->chunk.get() + offset;
        }
        dataSize = maxbound - where;
    }

    bool handlePath(SegmentTree* /* tree */,
        SimulatedPtrSize minbound,
        SimulatedPtrSize maxbound,
        SegmentNode::Ptr& t,
        SimulatedPtrSize where) {
        TRACE_FUNC;
        backTrace.push(t.get());

        TRACE_PARAM(where);
        auto available = maxbound - minbound;

        TRACE_PARAM(available);
        TRACE_PARAM(minbound);
        TRACE_PARAM(maxbound);
        TRACE_PARAM(t->status);
        TRACE_PARAM(t->state);

        if(handleMemState(t)) {
            dataSize = maxbound - where;
            return true;
        }
        return false;
    }
};

struct freeTraverser : emptyTraverser {
    SegmentNode::MemoryStatus desiredStatus;

    freeTraverser(SegmentNode::MemoryStatus desiredStatus): desiredStatus{desiredStatus} {}

    void handleEmptyNode(SimulatedPtr where) {
        TRACE_FUNC;
        signalIllegalFree(where);
    }

    bool handlePath(SegmentTree* /* tree */,
            SimulatedPtrSize minbound,
            SimulatedPtrSize /* maxbound*/,
            SegmentNode::Ptr& t,
            SimulatedPtrSize where){
        TRACE_FUNC;
        if(minbound == where && t->status == desiredStatus) {
            t.reset();
            return true;
        }
        return false;
    }

    bool handleChunk(SegmentTree* tree,
            SimulatedPtrSize minbound,
            SimulatedPtrSize maxbound,
            SegmentNode::Ptr& t,
            SimulatedPtrSize where) {
        TRACE_FUNC;
        return handlePath(tree, minbound, maxbound, t, where);
    }
};

struct memsetTraverser: stateInvalidatingTraverser {
    SimulatedPtr dest;
    uint8_t fill;
    SimulatedPtrSize size;
    memsetTraverser(SimulatedPtr dest, uint8_t fill, SimulatedPtrSize size):
        dest(dest), fill(fill), size(size) {};

    bool handleChunk(SegmentTree* tree,
            SimulatedPtrSize minbound,
            SimulatedPtrSize maxbound,
            SegmentNode::Ptr& t,
            SimulatedPtrSize where) {
        TRACE_FUNC;

        if(size == tree->chunk_size) {
            t->state = SegmentNode::MemoryState::Memset;
            t->memSetTo = fill;
            t->chunk.reset();
        } else if(size <= tree->chunk_size) {
            ASSERTC(where >= minbound);

            auto offset = where - minbound;
            ASSERTC(offset + size < maxbound);
            t->state = SegmentNode::MemoryState::Unknown;
            if(!t->chunk) t->chunk.reset(new uint8_t[tree->chunk_size]);

            auto realDst = t->chunk.get() + offset;
            std::memset(realDst, fill, size);
        }
        return true;
    }

    bool handlePath(SegmentTree* tree,
            SimulatedPtrSize minbound,
            SimulatedPtrSize maxbound,
            SegmentNode::Ptr& t,
            SimulatedPtrSize where) {
        TRACE_FUNC;

        TRACES() << "where: " << tfm::format("0x%x", where) << endl;

        auto available = maxbound - minbound;
        auto mid = middle(minbound, maxbound);

        TRACE_PARAM(available);
        TRACE_PARAM(minbound);
        TRACE_PARAM(maxbound);
        TRACE_PARAM(t->status);
        TRACE_PARAM(t->state);

        if(t->state != SegmentNode::MemoryState::Unknown
           && (minbound <= dest)
           && (maxbound >= dest + size)) {
            t->state = SegmentNode::MemoryState::Memset;
            t->memSetTo = fill;
            return true;
        } else if(where < mid && where + size > mid) {
            auto leftChunkSize = mid - where;
            auto rightChunkSize = size - leftChunkSize;

            forceChildrenAndDeriveState(*t);

            memsetTraverser left{ dest, fill, leftChunkSize };
            tree->traverse(where, left, t->left, minbound, mid);
            memsetTraverser right{ dest + leftChunkSize, fill, rightChunkSize };
            tree->traverse(mid, right, t->right, mid, maxbound);
        } else return false;
        return true;
    }
};

} /* empty namespace */

void SegmentTree::allocate(
        SimulatedPtr where,
        SimulatedPtrSize size,
        SegmentNode::MemoryState state,
        SegmentNode::MemoryStatus status) {
    TRACE_FUNC;
    TRACE_PARAM(where);
    TRACE_PARAM(size);
    TRACE_PARAM(state);
    TRACE_PARAM(status);

    if(size > (end - start)) signalOutOfMemory(size);

    traverse(where, allocTraverser{state, status, size});
}

void SegmentTree::store(
        SimulatedPtr where,
        const uint8_t* data,
        SimulatedPtrSize size) {
    TRACE_FUNC;
    TRACE_PARAM(where);
    TRACE_PARAM((void*)data);
    TRACE_PARAM(size);

    return traverse(where, storeTraverser{ data, size, false });
}

SegmentTree::intervalState SegmentTree::get(SimulatedPtr where) {
    TRACE_FUNC;

    loadTraverser loadTraverser;
    traverse(where, loadTraverser);

    return { loadTraverser.ptr, loadTraverser.state, loadTraverser.filledWith };
}

SimulatedPtr SegmentTree::memchr(SimulatedPtr where, uint8_t ch, size_t limit) {
    TRACE_FUNC;

    TRACE_PARAM(where);
    TRACE_PARAM(+ch);
    TRACE_PARAM(limit);

    bool noLimit = (limit == ~size_t(0));
    long long mutableLimit = limit;

    while(noLimit || mutableLimit > 0){
        loadTraverser loadTraverser;
        traverse(where, loadTraverser);

        auto sz = noLimit? loadTraverser.dataSize :
            std::min(mutableLimit, static_cast<long long>(loadTraverser.dataSize));

        if(loadTraverser.state == SegmentNode::MemoryState::Memset) {
            if(ch == loadTraverser.filledWith) return where;
        } else if(loadTraverser.state == SegmentNode::MemoryState::Uninit) {
            signalIllegalLoad(where);
        } else {
            if(auto memloc = std::memchr(loadTraverser.ptr, ch, sz)) {
                return where + (static_cast<uint8_t*>(memloc) - loadTraverser.ptr);
            }
        }
        // no symbol found, go next
        where += sz;
        mutableLimit -= sz;
    }

    return 0;
}

void SegmentTree::free(SimulatedPtr where, SegmentNode::MemoryStatus desiredStatus) {
    TRACE_FUNC;

    if(where < start || where >= end) signalIllegalFree(where);
    return traverse(where, freeTraverser{desiredStatus});
}

void SegmentTree::memset(SimulatedPtr where, uint8_t fill, SimulatedPtrSize size) {
    TRACE_FUNC;

    if(where < start || where >= end) signalIllegalStore(where);

    return traverse(where, memsetTraverser{ where, fill, size });

}

} /* namespace borealis */

#include "Util/unmacros.h"
