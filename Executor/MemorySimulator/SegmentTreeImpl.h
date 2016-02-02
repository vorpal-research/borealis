/*
 * SegmentTreeImpl.h
 *
 *  Created on: Mar 4, 2015
 *      Author: belyaev
 */

#ifndef EXECUTOR_MEMORYSIMULATOR_SEGMENTTREEIMPL_H_
#define EXECUTOR_MEMORYSIMULATOR_SEGMENTTREEIMPL_H_

#include <cstdint>
#include <memory>
#include <cstring>
#include <unordered_map>

#include <tinyformat/tinyformat.h>

#include "Executor/Exceptions.h"
#include "Util/util.h"
#include "Config/config.h"

#include "Logging/tracer.hpp"

#include "Util/macros.h"

namespace borealis {

using ChunkType = std::unique_ptr<uint8_t[]>;
using SimulatedPtr = std::uintptr_t;
using SimulatedPtrSize = std::uintptr_t;

static config::ConfigEntry<int> K{"executor", "memory-chunk-multiplier"};
static config::ConfigEntry<int> M{"executor", "memory-power"};

inline SimulatedPtr ptr_cast(const void* vd) {
    return reinterpret_cast<SimulatedPtr>(vd);
}

inline SimulatedPtr ptr_cast(const uint8_t* vd) {
    return reinterpret_cast<SimulatedPtr>(vd);
}

inline void* ptr_cast(SimulatedPtr vd) {
    return reinterpret_cast<void*>(vd);
}

inline SimulatedPtr middle(SimulatedPtr from, SimulatedPtr to) {
    return from + (to - from)/2;
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

    inline void allocateChunk(size_t chunk_size) {
        if(!chunk) chunk.reset(new uint8_t[chunk_size]);
    }
    inline void deallocateChunk() {
        chunk.reset();
    }
};

inline std::ostream& operator<<(std::ostream& ost, SegmentNode::MemoryStatus st) {
    switch(st) {
    case SegmentNode::MemoryStatus::Unallocated: return ost << "Unallocated";
    case SegmentNode::MemoryStatus::Malloc:      return ost << "Malloc";
    case SegmentNode::MemoryStatus::Alloca:      return ost << "Alloca";
    }
}

inline std::ostream& operator<<(std::ostream& ost, SegmentNode::MemoryState st) {
    switch(st) {
    case SegmentNode::MemoryState::Memset:   return ost << "Memset";
    case SegmentNode::MemoryState::Uninit:   return ost << "Uninit";
    case SegmentNode::MemoryState::Unknown:  return ost << "Unknown";
    }
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

static void signalOutOfMemory(SimulatedPtrSize) {
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
        TRACE_FUNC;
        TRACE_PARAM(t.state);
        TRACE_PARAM((size_t)t.memSetTo);
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
        TRACE_PARAM(t.state);
        forceChildrenAndDeriveState(t);
    }

    void handleGoRight(SegmentNode& t) {
        TRACE_FUNC;
        TRACE_PARAM(t.state);
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
        TRACE_PARAM(where);
        TRACE_PARAM(t.get());
        TRACE_PARAM(minbound);
        TRACE_PARAM(maxbound);

        ASSERTC(maxbound > minbound);
        ASSERTC(where >= minbound);
        ASSERTC(where < maxbound);

        if(!t) theTraverser.handleEmptyNode(where);

        auto available = maxbound - minbound;
        auto mid = middle(minbound, maxbound);
        TRACE_PARAM(available);
        TRACE_PARAM(mid);

        force(t);
        if(available <= chunk_size) {
            theTraverser.handleChunk(this, minbound, maxbound, t, where);
            return;
        }

        if(theTraverser.handlePath(this, minbound, maxbound, t, where)) {
            return;
        }

        if(where >= minbound && where < mid) {
            theTraverser.handleGoLeft(*t);
            traverse<Traverser>( where, theTraverser, t->left, minbound, mid);
        } else if(where >= mid && where < maxbound) {
            theTraverser.handleGoRight(*t);
            traverse<Traverser>(where, theTraverser, t->right, mid, maxbound);
        } else {
            signalInconsistency(__PRETTY_FUNCTION__);
        }
    }

    struct intervalState {
        uint8_t* data;
        SegmentNode::MemoryState memState;
        uint8_t filledWith;
    };

    void allocate(SimulatedPtr where, SimulatedPtrSize size,
        SegmentNode::MemoryState state, SegmentNode::MemoryStatus status);
    void store(SimulatedPtr where, const uint8_t* data, SimulatedPtrSize size);
    intervalState get(SimulatedPtr where);
    SimulatedPtr memchr(SimulatedPtr where, uint8_t ch, size_t limit);
    void free(SimulatedPtr where, SegmentNode::MemoryStatus desiredStatus);
    void memset(SimulatedPtr where, uint8_t fill, SimulatedPtrSize size);

    struct memIntervalInfo {
        uint8_t* data;
        SegmentNode::MemoryState memState;
        uint8_t filledWith;

        SimulatedPtr start;
        SimulatedPtr end;

        SimulatedPtrSize size() const {
            return end - start;
        }

        memIntervalInfo(uint8_t *data, SegmentNode::MemoryState memState,
            uint8_t filledWith, SimulatedPtr start, SimulatedPtr end)
                : data(data), memState(memState), filledWith(filledWith), start(start), end(end) {
        }

        bool mergeIn(const memIntervalInfo& other) {
            if(this->memState != other.memState) return false;
            if(this->memState == SegmentNode::MemoryState::Unknown) return false;

            if(this->memState == SegmentNode::MemoryState::Memset
                && this->filledWith != other.filledWith) return false;

            end = other.end;
            return true;
        }

        static void mergeAdjacent(std::list<memIntervalInfo>& lst) {
            auto&& it = std::begin(lst);
            auto&& end = std::end(lst);
            while(it != end) {
                auto next = std::next(it);
                if(next == end) break;

                auto merged = it->mergeIn(*next);
                if(merged) lst.erase(next);
                else it = next;
            }
        }
    };


    void memmove(SimulatedPtr src, SimulatedPtr dst, SimulatedPtrSize sz) {

        struct intervalCollector: emptyTraverser {
            SimulatedPtr src;
            SimulatedPtrSize sz;
            std::list<memIntervalInfo>* path;

            intervalCollector(SimulatedPtr src, SimulatedPtrSize sz, std::list<memIntervalInfo> *path)
                    : src(src), sz(sz), path(path) {
            }

            void handleEmptyNode(SimulatedPtrSize where) {
                TRACE_FUNC;
                signalIllegalLoad(where);
            }

            bool handleEthereal(
                SegmentTree* /* tree */,
                SimulatedPtrSize minbound,
                SimulatedPtrSize maxbound,
                SegmentNode::Ptr& t,
                SimulatedPtrSize where
            ) {
                TRACE_FUNC;
                if(t->state != SegmentNode::MemoryState::Unknown) {
                     path->emplace_back(
                         nullptr,
                         t->state,
                         t->memSetTo,
                         std::max(minbound, where),
                         std::min(maxbound, where + sz)
                     );
                     return true;
                }
                return false;
            }

            void handleChunk(
                SegmentTree* tree,
                SimulatedPtrSize minbound,
                SimulatedPtrSize maxbound,
                SegmentNode::Ptr& t,
                SimulatedPtrSize where
            ) {
                TRACE_FUNC;

                if(!handleEthereal(tree, minbound, maxbound, t, where)) {
                    auto startOffset = where - minbound;

                    path->emplace_back(
                        t->chunk.get() + startOffset,
                        SegmentNode::MemoryState::Unknown,
                        0,
                        std::max(minbound, where),
                        std::min(maxbound, where + sz)
                    );
                }

            }

            bool handlePath(
                SegmentTree* tree,
                SimulatedPtrSize minbound,
                SimulatedPtrSize maxbound,
                SegmentNode::Ptr& t,
                SimulatedPtrSize where
            ) {
                TRACE_FUNC;
                TRACE_FMT("where: 0x%x", where);

                // auto available = maxbound - minbound;
                auto mid = middle(minbound, maxbound);

                if(handleEthereal(tree, minbound, maxbound, t, where)) {
                    return true;
                } else if(where < mid && (where + sz) > mid) {
                    auto leftChunkSize = mid - where;
                    auto rightChunkSize = sz - leftChunkSize;

                    intervalCollector left{ src, leftChunkSize, path };
                    tree->traverse(where, left, t->left, minbound, mid);
                    intervalCollector right{ src + leftChunkSize, rightChunkSize, path };
                    tree->traverse(mid, right, t->right, mid, maxbound);
                } else return false;

                return true;
            }
        };

        std::list<memIntervalInfo> sourceInterval;

        traverse(src, intervalCollector{ src, sz, &sourceInterval });
        memIntervalInfo::mergeAdjacent(sourceInterval);

        struct mergeInTraverser: stateInvalidatingTraverser {
            SimulatedPtr dst;
            memIntervalInfo path;
            SimulatedPtrSize sz;

            mergeInTraverser(SimulatedPtr dst, const memIntervalInfo& path)
                    : dst(dst), path(path), sz(path.size()) {
            }

            void handleChunk(
                            SegmentTree* /* tree */,
                            SimulatedPtrSize minbound,
                            SimulatedPtrSize maxbound,
                            SegmentNode::Ptr& t,
                            SimulatedPtrSize where
                        ) {
                TRACE_FUNC;
                if(path.memState != SegmentNode::MemoryState::Unknown) {
                    ASSERTC(where >= minbound);

                    auto offset = where - minbound;
                    ASSERTC(offset + sz < maxbound);

                    auto realDst = t->chunk.get() + offset;

                    std::memset(realDst, path.filledWith, sz);
                } else if(path.memState == SegmentNode::MemoryState::Unknown) {
                    ASSERTC(where >= minbound);

                    auto offset = where - minbound;
                    ASSERTC(offset + sz < maxbound);

                    auto realDst = t->chunk.get() + offset;
                    auto realSrc = path.data;

                    std::memcpy(realDst, realSrc, sz);
                }
            }

            bool handlePath(
                SegmentTree* tree,
                SimulatedPtrSize minbound,
                SimulatedPtrSize maxbound,
                SegmentNode::Ptr& t,
                SimulatedPtrSize where
            ) {
                TRACE_FUNC;
                TRACE_FMT("where: 0x%x", where);

                auto available = maxbound - minbound;
                auto mid = middle(minbound, maxbound);

                TRACE_PARAM(available);
                TRACE_PARAM(minbound);
                TRACE_PARAM(maxbound);
                TRACE_PARAM(t->status);
                TRACE_PARAM(t->state);

                // FIXME!
                if(available == sz && path.memState != SegmentNode::MemoryState::Unknown) {
                    t->state = path.memState;
                    t->memSetTo = path.filledWith;
                } else if(where < mid && (where + sz) > mid) {
                     auto leftChunkSize = mid - where;
                     auto rightChunkSize = sz - leftChunkSize;

                     forceChildrenAndDeriveState(*t);

                     memIntervalInfo leftInterval = path;
                     leftInterval.end -= rightChunkSize;
                     mergeInTraverser left{ dst, leftInterval };
                     tree->traverse(where, left, t->left, minbound, mid);

                     memIntervalInfo rightInterval = path;
                     rightInterval.start += leftChunkSize;
                     mergeInTraverser right{ dst + leftChunkSize, rightInterval };
                     tree->traverse(mid, right, t->right, mid, maxbound);
                } else return false;

                return true;
            }
        };

        auto mutableDst = dst;
        // FIXME: this seems pretty inefficient
        for(auto&& memPiece: sourceInterval) {
            traverse(mutableDst, mergeInTraverser{ mutableDst, memPiece });
            mutableDst += memPiece.size();
        }

    }

};


} /* namespace borealis */

namespace llvm {

template<>
struct GraphTraits<borealis::SegmentTree*> {
    // TODO
};

}

#include "Util/unmacros.h"

#endif /* EXECUTOR_MEMORYSIMULATOR_SEGMENTTREEIMPL_H_ */
