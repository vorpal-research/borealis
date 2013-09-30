/*
 * ExecutionContext.h
 *
 *  Created on: Nov 22, 2012
 *      Author: belyaev
 */

#ifndef Z3_EXECUTIONCONTEXT_H_
#define Z3_EXECUTIONCONTEXT_H_

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "SMT/Z3/ExprFactory.h"

namespace borealis {
namespace z3_ {

class ExecutionContext {

    USING_SMT_LOGIC(Z3);
    typedef Z3::ExprFactory ExprFactory;

    ExprFactory& factory;
    mutable std::unordered_map<std::string, MemArray> memArrays;
    unsigned long long currentPtr;

    static constexpr auto MEMORY_ID = "$$__MEMORY__$$";
    MemArray memory() const {
        return get(MEMORY_ID);
    }
    void memory(const MemArray& value) {
        set(MEMORY_ID, value);
    }

    static constexpr auto GEP_BOUNDS_ID = "$$__gep_bound__$$";
    MemArray gepBounds() const {
        return get(GEP_BOUNDS_ID);
    }
    void gepBounds(const MemArray& value) {
        set(GEP_BOUNDS_ID, value);
    }

    MemArray get(const std::string& id) const {
        using borealis::util::containsKey;
        if (containsKey(memArrays, id)) {
            return memArrays.at(id);
        }
        auto ret = factory.getNoMemoryArray();
        memArrays.emplace(id, ret);
        return ret;
    }
    void set(const std::string& id, const MemArray& value) {
        using borealis::util::containsKey;
        if (containsKey(memArrays, id)) {
            memArrays.erase(id);
        }
        memArrays.emplace(id, value);
    }

    typedef std::unordered_set<std::string> MemArrayIds;
    MemArrayIds getMemArrayIds() const {
        auto it = util::iterate_keys(util::begin_end_pair(memArrays));
        return MemArrayIds(it.first, it.second);
    }

public:

    ExecutionContext(ExprFactory& factory);
    ExecutionContext(const ExecutionContext&) = default;

    MemArray getCurrentMemoryContents() {
        return memory();
    }
    MemArray getCurrentGepBounds() {
        return gepBounds();
    }

    inline Pointer getDistinctPtr(size_t offsetSize = 1U) {
        auto res = factory.getPtrConst(currentPtr);
        currentPtr += offsetSize;
        gepBounds( gepBounds().store(res, factory.getPtrConst(offsetSize)) );
        return res;
    }

////////////////////////////////////////////////////////////////////////////////

    Dynamic readExprFromMemory(Pointer ix, size_t bitSize) {
        return memory().select(ix, bitSize);
    }
    template<class ExprClass>
    ExprClass readExprFromMemory(Pointer ix) {
        return memory().select<ExprClass>(ix);
    }
    template<class ExprClass>
    void writeExprToMemory(Pointer ix, ExprClass val) {
        memory( memory().store(ix, val) );
    }

    Dynamic readProperty(const std::string& id, Pointer ix, size_t bitSize) {
        return get(id).select(ix, bitSize);
    }
    template<class ExprClass>
    ExprClass readProperty(const std::string& id, Pointer ix) {
        return get(id).select<ExprClass>(ix);
    }
    template<class ExprClass>
    void writeProperty(const std::string& id, Pointer ix, ExprClass val) {
        set( id, get(id).store(ix, val) );
    }

////////////////////////////////////////////////////////////////////////////////

    typedef std::pair<Bool, ExecutionContext> Choice;

    ExecutionContext& switchOn(
            const std::string& name,
            const std::vector<Choice>& contexts) {
        auto merged = ExecutionContext::mergeMemory(name, *this, contexts);

        this->memArrays = merged.memArrays;
        this->currentPtr = merged.currentPtr;

        return *this;
    }

    static ExecutionContext mergeMemory(
            const std::string& name,
            ExecutionContext defaultContext,
            const std::vector<Choice>& contexts) {
        ExecutionContext res(defaultContext.factory);

        // Merge current pointer
        res.currentPtr = defaultContext.currentPtr;
        for (const auto& e : contexts) {
            res.currentPtr = std::max(res.currentPtr, e.second.currentPtr);
        }

        // Collect all active memory array ids
        auto memArrayIds = std::accumulate(contexts.begin(), contexts.end(),
            defaultContext.getMemArrayIds(),
            [](MemArrayIds a, Choice e) -> MemArrayIds {
                auto ids = e.second.getMemArrayIds();
                a.insert(ids.begin(), ids.end());
                return a;
            }
        );

        // Merge memory arrays
        for (const auto& id : memArrayIds) {
            std::vector<std::pair<Bool, MemArray>> alternatives;
            alternatives.reserve(contexts.size());
            std::transform(contexts.begin(), contexts.end(), std::back_inserter(alternatives),
                [&id](const Choice& p) { return std::make_pair(p.first, p.second.get(id)); }
            );

            res.set(id, MemArray::merge(name, defaultContext.get(id), alternatives));
        }

        return res;
    }

////////////////////////////////////////////////////////////////////////////////

    Pointer getBound(const Pointer& p) {
        return readProperty<Pointer>(GEP_BOUNDS_ID, p);
    }

////////////////////////////////////////////////////////////////////////////////

    Bool toSMT() const;

    friend std::ostream& operator<<(std::ostream& s, const ExecutionContext& ctx);
};

std::ostream& operator<<(std::ostream& s, const ExecutionContext& ctx);

} // namespace z3_
} // namespace borealis

#endif /* Z3_EXECUTIONCONTEXT_H_ */
