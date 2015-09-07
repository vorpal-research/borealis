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
    using ExprFactory = Z3::ExprFactory;

    ExprFactory& factory;
    mutable std::unordered_map<std::string, MemArray> memArrays;
    mutable std::unordered_map<std::string, MemArray> initialMemArrays;
    unsigned long long globalPtr;
    unsigned long long localPtr;

    unsigned long long localMemoryStart;
    unsigned long long localMemoryEnd;

    std::vector<Bool> contextAxioms;

    static const std::string MEMORY_ID;
    MemArray memory() const;
    void memory(const MemArray& value);

    static const std::string GEP_BOUNDS_ID;
    void initGepBounds();
    MemArray gepBounds() const;
    void gepBounds(const MemArray& value);

    MemArray get(const std::string& id) const;
    void set(const std::string& id, const MemArray& value);

    using MemArrayIds = std::unordered_set<std::string>;
    MemArrayIds getMemArrayIds() const;

public:

    ExecutionContext(ExprFactory& factory, unsigned long long localMemoryStart, unsigned long long localMemoryEnd);
    ExecutionContext(const ExecutionContext&) = default;

    MemArray getCurrentMemoryContents();
    MemArray getCurrentGepBounds();

    MemArray getInitialMemoryContents();

    Pointer getGlobalPtr(size_t offsetSize = 1U);
    Pointer getGlobalPtr(size_t offsetSize, Integer origSize);

    Pointer getLocalPtr(size_t offsetSize = 1U);
    Pointer getLocalPtr(size_t offsetSize, Integer origSize);

    using LocalMemoryBounds = std::pair<unsigned long long, unsigned long long>;
    LocalMemoryBounds getLocalMemoryBounds() const;

    const std::vector<Bool>& getAxioms() const { return contextAxioms; }

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
    template<class ExprClass>
    void writeExprRangeToMemory(Pointer from, size_t size, ExprClass val) {
        auto&& currentMemory = memory();

        auto&& newMem = factory.getEmptyMemoryArray(MEMORY_ID);

        std::function<Bool(Pointer)> fun = [=](Pointer inner) {
            return
                factory.if_(inner >= from && inner < from + size)
                       .then_(newMem.select<ExprClass>(inner) == val)
                       .else_(newMem.select<ExprClass>(inner) == currentMemory.select<ExprClass>(inner));
        };
        std::function<std::vector<Dynamic>(Pointer)> patterns = [=](Pointer inner) {
            return util::make_vector(Dynamic(newMem.select<ExprClass>(inner)));
        };

        auto&& axiom = factory.forAll(fun, patterns);

        contextAxioms.push_back(axiom);

        return memory(newMem);
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

    using Choice = std::pair<Bool, ExecutionContext>;

    ExecutionContext& switchOn(const std::string& name, const std::vector<Choice>& contexts);

    static ExecutionContext mergeMemory(
            const std::string& name,
            ExecutionContext defaultContext,
            const std::vector<Choice>& contexts);

////////////////////////////////////////////////////////////////////////////////

    Integer getBound(const Pointer& p);
    void writeBound(const Pointer& p, const Integer& bound);

////////////////////////////////////////////////////////////////////////////////

    Bool toSMT() const;

    friend std::ostream& operator<<(std::ostream& s, const ExecutionContext& ctx);

};

} // namespace z3_
} // namespace borealis

#endif /* Z3_EXECUTIONCONTEXT_H_ */
