/*
 * Z3Context.h
 *
 *  Created on: Nov 22, 2012
 *      Author: belyaev
 */

#ifndef Z3CONTEXT_H_
#define Z3CONTEXT_H_

#include <z3/z3++.h>

#include <vector>

#include "Solver/Z3ExprFactory.h"

namespace borealis {

class ExecutionContext {
public:
    typedef Z3ExprFactory::Bool Bool;
    typedef Z3ExprFactory::Pointer Pointer;
    typedef Z3ExprFactory::Dynamic Dynamic;
    typedef Z3ExprFactory::MemArray MemArray;

private:
    Z3ExprFactory& factory;
    MemArray memory;
    unsigned long long currentPtr;

public:
    ExecutionContext(Z3ExprFactory& factory);

    MemArray getCurrentMemoryContents() {
        TRACE_FUNC;
        return memory;
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

    Dynamic readExprFromMemory(Pointer ix, size_t bitSize) {
        TRACE_FUNC;
        return memory.select(ix, bitSize);
    }

    template<class ExprClass>
    ExprClass readExprFromMemory(Pointer ix) {
        TRACE_FUNC;
        return memory.select<ExprClass>(ix);
    }

    template<class ExprClass>
    void writeExprToMemory(Pointer ix, ExprClass val) {
        TRACE_FUNC;
        memory = memory.store(ix, val);
    }

    static ExecutionContext mergeMemory(
            ExecutionContext defaultContext,
            const std::vector<std::pair<Bool, ExecutionContext>>&  contexts) {
        ExecutionContext res(defaultContext.factory);

        // Merge current pointer
        res.currentPtr = defaultContext.currentPtr;
        for (auto& e : contexts) {
            res.currentPtr = std::max(res.currentPtr, e.second.currentPtr);
        }

        // Merge memory
        std::vector<std::pair<Bool, MemArray>> memories;
        memories.reserve(contexts.size());
        std::transform(contexts.begin(), contexts.end(), std::back_inserter(memories),
            [](const std::pair<Bool, ExecutionContext>& p) {
                return std::make_pair(p.first, p.second.memory);
            }
        );
        res.memory = MemArray::merge(defaultContext.memory, memories);

        return res;
    }

    Bool toZ3();
};

} /* namespace borealis */

#endif /* Z3CONTEXT_H_ */
