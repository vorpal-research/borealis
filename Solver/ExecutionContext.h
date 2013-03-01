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
    typedef Z3ExprFactory::Pointer Pointer;
    typedef Z3ExprFactory::Dynamic Dynamic;
    typedef Z3ExprFactory::MemArray MemArray;

private:
    Z3ExprFactory& factory;
    MemArray memory;
    std::vector<Z3ExprFactory::Pointer> allocated_pointers;

public:
    ExecutionContext(Z3ExprFactory& factory);

    MemArray getCurrentMemoryContents() { return memory; }

    inline void registerDistinctPtr(Z3ExprFactory::Pointer ptr) {
        allocated_pointers.push_back(ptr);
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

    logic::Bool toZ3();
};

} /* namespace borealis */

#endif /* Z3CONTEXT_H_ */
