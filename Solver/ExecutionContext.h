/*
 * Z3Context.h
 *
 *  Created on: Nov 22, 2012
 *      Author: belyaev
 */

#ifndef Z3CONTEXT_H_
#define Z3CONTEXT_H_

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Logging/tracer.hpp"
#include "Solver/Z3ExprFactory.h"

namespace borealis {

class ExecutionContext {
public:
    typedef Z3ExprFactory::Bool Bool;
    typedef Z3ExprFactory::Pointer Pointer;
    typedef Z3ExprFactory::MemArray MemArray;
    typedef Z3ExprFactory::Dynamic Dynamic;

private:
    Z3ExprFactory& factory;
    std::unordered_map<std::string, MemArray> memArrays;
    unsigned long long currentPtr;

    static constexpr auto MEMORY_ID = "$$__MEMORY__$$";
    MemArray memory() const {
        return get(MEMORY_ID);
    }
    void memory(const MemArray& value) {
        set(MEMORY_ID, value);
    }

    MemArray get(const std::string& id) const {
        using borealis::util::containsKey;
        if (containsKey(memArrays, id)) {
            return memArrays.at(id);
        }
        return factory.getNoMemoryArray();
    }
    void set(const std::string& id, const MemArray& value) {
        memArrays.emplace(id, value);
    }

    typedef std::unordered_set<std::string> MemArrayIds;

    MemArrayIds getMemArrayIds() const {
        auto it = util::iterate_keys(util::begin_end_pair(memArrays));
        return MemArrayIds(it.first, it.second);
    }

public:
    ExecutionContext(Z3ExprFactory& factory);
    ExecutionContext(const ExecutionContext&) = default;

    MemArray getCurrentMemoryContents() {
        TRACE_FUNC;
        return memory();
    }

    inline Pointer getDistinctPtr(size_t ofSize = 1U) {
        TRACE_FUNC;

        auto ret = Pointer::mkConst(
            factory.unwrap(),
            currentPtr
        );

        currentPtr += ofSize;

        return ret;
    }

////////////////////////////////////////////////////////////////////////////////

    Dynamic readExprFromMemory(Pointer ix, size_t bitSize) {
        TRACE_FUNC;
        return memory().select(ix, bitSize);
    }
    template<class ExprClass>
    ExprClass readExprFromMemory(Pointer ix) {
        TRACE_FUNC;
        return memory().select<ExprClass>(ix);
    }
    template<class ExprClass>
    void writeExprToMemory(Pointer ix, ExprClass val) {
        TRACE_FUNC;
        memory( memory().store(ix, val) );
    }

    Dynamic readProperty(const std::string& id, Pointer ix, size_t bitSize) {
        TRACE_FUNC;
        return get(id).select(ix, bitSize);
    }
    template<class ExprClass>
    ExprClass readProperty(const std::string& id, Pointer ix) {
        TRACE_FUNC;
        return get(id).select<ExprClass>(ix);
    }
    template<class ExprClass>
    void writeProperty(const std::string& id, Pointer ix, ExprClass val) {
        TRACE_FUNC;
        set( id, get(id).store(ix, val) );
    }

////////////////////////////////////////////////////////////////////////////////

    typedef std::pair<Bool, ExecutionContext> Choice;

    ExecutionContext& switchOn(const std::vector<Choice>& contexts) {
        auto merged = ExecutionContext::mergeMemory(*this, contexts);

        this->memArrays = merged.memArrays;
        this->currentPtr = merged.currentPtr;

        return *this;
    }

    static ExecutionContext mergeMemory(
            ExecutionContext defaultContext,
            const std::vector<Choice>& contexts) {
        ExecutionContext res(defaultContext.factory);

        // Merge current pointer
        res.currentPtr = defaultContext.currentPtr;
        for (auto& e : contexts) {
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

            res.set(id, MemArray::merge(defaultContext.get(id), alternatives));
        }

        return res;
    }

    Bool toZ3();

    friend std::ostream& operator<<(std::ostream& s, const ExecutionContext& ctx);
};

std::ostream& operator<<(std::ostream& s, const ExecutionContext& ctx);

} /* namespace borealis */

#endif /* Z3CONTEXT_H_ */
