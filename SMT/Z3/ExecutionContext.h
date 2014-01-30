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
    unsigned long long globalPtr;
    unsigned long long localPtr;

    static std::string MEMORY_ID;
    MemArray memory() const {
        return get(MEMORY_ID);
    }
    void memory(const MemArray& value) {
        set(MEMORY_ID, value);
    }

    static std::string GEP_BOUNDS_ID;
    void initGepBounds() {
        memArrays.emplace(
            GEP_BOUNDS_ID,
            factory.getDefaultMemoryArray(GEP_BOUNDS_ID, 0)
        );
        gepBounds( gepBounds().store(
            factory.getNullPtr(),
            factory.getIntConst(-1)
        ) );
    }
    MemArray gepBounds() const {
        return get(GEP_BOUNDS_ID);
    }
    void gepBounds(const MemArray& value) {
        set(GEP_BOUNDS_ID, value);
    }

    MemArray get(const std::string& id) const {
        using borealis::util::containsKey;
        if (!containsKey(memArrays, id)) {
            memArrays.emplace(id, factory.getNoMemoryArray(id));
        }
        return memArrays.at(id);
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

    ExecutionContext(ExprFactory& factory, unsigned long long localMemory);
    ExecutionContext(const ExecutionContext&) = default;

    MemArray getCurrentMemoryContents() {
        return memory();
    }
    MemArray getCurrentGepBounds() {
        return gepBounds();
    }

    inline Pointer getGlobalPtr(size_t offsetSize = 1U) {
        return getGlobalPtr(offsetSize, factory.getIntConst(offsetSize));
    }
    inline Pointer getGlobalPtr(size_t offsetSize, Integer origSize) {
        auto ret = factory.getPtrConst(globalPtr);
        globalPtr += offsetSize;
        gepBounds( gepBounds().store(ret, origSize) );
        return ret;
    }

    inline Pointer getLocalPtr(size_t offsetSize = 1U) {
        return getLocalPtr(offsetSize, factory.getIntConst(offsetSize));
    }
    inline Pointer getLocalPtr(size_t offsetSize, Integer origSize) {
        auto ret = factory.getPtrConst(localPtr);
        localPtr += offsetSize;
        gepBounds( gepBounds().store(ret, origSize) );
        return ret;
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
        this->globalPtr = merged.globalPtr;
        this->localPtr = merged.localPtr;

        return *this;
    }

    static ExecutionContext mergeMemory(
            const std::string& name,
            ExecutionContext defaultContext,
            const std::vector<Choice>& contexts) {
        ExecutionContext res(defaultContext.factory, 0ULL);

        // Merge pointers
        for (const auto& e : contexts) {
            res.globalPtr = std::max(res.globalPtr, e.second.globalPtr);
            res.localPtr = std::max(res.localPtr, e.second.localPtr);
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

    Integer getBound(const Pointer& p) {

        auto base = factory.getPtrVar("$$__base__$$(" + p.getName() + ")");
        auto zero = factory.getIntConst(0);

        auto baseSize = readProperty<Integer>(GEP_BOUNDS_ID, base);
        auto pSize =  readProperty<Integer>(GEP_BOUNDS_ID, p);

        std::function<Bool(Pointer)> axBody =
            [=](Pointer any) -> Bool {
                return factory.implies(
                    UComparable(any).ugt(base) && UComparable(any).ule(p),
                    readProperty<Integer>(GEP_BOUNDS_ID, any) == zero
                );
            };
        auto ax = UComparable(base).ule(p) &&
                  factory.if_(pSize != zero)
                         .then_(base == p)
                         .else_(
                             baseSize != zero &&
                             factory.forAll(axBody)
                         );
        base = base.withAxiom(ax);

        auto pBound = baseSize - (p - base);

        return factory.if_(pBound >= zero)
                .then_(pBound)
                .else_(zero);
    }

////////////////////////////////////////////////////////////////////////////////

    Bool toSMT() const;

    friend std::ostream& operator<<(std::ostream& s, const ExecutionContext& ctx);
};

std::ostream& operator<<(std::ostream& s, const ExecutionContext& ctx);

} // namespace z3_
} // namespace borealis

#endif /* Z3_EXECUTIONCONTEXT_H_ */
