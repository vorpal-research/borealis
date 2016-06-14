/*
 * ExecutionContext.h
 *
 *  Created on: Aug 5, 2013
 *      Author: sam
 */

#ifndef MATHSAT_EXECUTIONCONTEXT_H_
#define MATHSAT_EXECUTIONCONTEXT_H_

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "SMT/MathSAT/ExprFactory.h"

namespace borealis {
namespace mathsat_ {

class ExecutionContext {

    USING_SMT_LOGIC(MathSAT);
    using ExprFactory = MathSAT::ExprFactory;

    ExprFactory& factory;
    mutable std::unordered_map<std::string, MemArray> memArrays;
    unsigned long long globalPtr;
    unsigned long long localPtr;

    unsigned long long localMemoryStart;
    unsigned long long localMemoryEnd;

    static const std::string MEMORY_ID;
    MemArray memory() const;
    void memory(const MemArray& value);

    static const std::string GEP_BOUNDS_ID;
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

    Pointer getGlobalPtr(size_t offsetSize = 1U);
    Pointer getGlobalPtr(size_t offsetSize, Integer origSize);

    Pointer getLocalPtr(size_t offsetSize = 1U);
    Pointer getLocalPtr(size_t offsetSize, Integer origSize);

    using LocalMemoryBounds = std::pair<unsigned long long, unsigned long long>;
    LocalMemoryBounds getLocalMemoryBounds() const;

////////////////////////////////////////////////////////////////////////////////

    Dynamic readExprFromMemory(Pointer ix, size_t bitSize) {
        return memory().select(ix, bitSize);
    }
    template<class ExprClass>
    ExprClass readExprFromMemory(Pointer ix) {
        return memory().select<ExprClass>(ix);
    }
    void writeExprToMemory(Pointer ix, DynBV val) {
        writeExprToMemory(ix, Byte::forceCast(val));
    }
    template<class ExprClass>
    void writeExprToMemory(Pointer ix, ExprClass val) {
        memory( memory().store(ix, val) );
    }
    void writeExprRangeToMemory(Pointer from, size_t size, DynBV val) {
        writeExprRangeToMemory(from, size, Byte::forceCast(val));
    }
    template<class ExprClass>
    void writeExprRangeToMemory(Pointer from, size_t size, ExprClass val) {
        for (auto&& i = 0U; i < size; ++i) {
            writeExprToMemory(from + i, val);
        }
    }

    Dynamic readProperty(const std::string& id, Pointer ix, size_t bitSize) {
        return get(id).select(ix, bitSize);
    }
    template<class ExprClass>
    ExprClass readProperty(const std::string& id, Pointer ix) {
        return get(id).select<ExprClass>(ix);
    }
    void writeProperty(const std::string& id, Pointer ix, DynBV val) {
        writeProperty(id, ix, Byte::forceCast(val));
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

    Integer getBound(const Pointer& p, size_t bitSize);
    void writeBound(const Pointer& p, const Integer& bound);

////////////////////////////////////////////////////////////////////////////////

    Bool toSMT() const;

    friend std::ostream& operator<<(std::ostream& s, const ExecutionContext& ctx);

};

} // namespace mathsat_
} // namespace borealis

#endif /* MATHSAT_EXECUTIONCONTEXT_H_ */
