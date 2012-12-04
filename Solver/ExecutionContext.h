/*
 * Z3Context.h
 *
 *  Created on: Nov 22, 2012
 *      Author: belyaev
 */

#ifndef Z3CONTEXT_H_
#define Z3CONTEXT_H_

#include <vector>

#include <z3/z3++.h>

#include "Solver/Z3ExprFactory.h"

namespace borealis {

class ExecutionContext {
    typedef Z3ExprFactory::expr expr;
    typedef Z3ExprFactory::array array;

    Z3ExprFactory& factory;
    array memory;
    Z3ExprFactory::exprRef axiom;

    std::vector<expr> allocated_pointers;
public:
    ExecutionContext(Z3ExprFactory& factory);

    array getCurrentMemoryContents() { return memory; }

    inline void registerDistinctPtr(expr ptr) {
        allocated_pointers.push_back(ptr);
    }

    expr readExprFromMemory(expr ix, size_t sz);
    void writeExprToMemory(expr ix, expr val);
    void mutateMemory( const std::function<array(array)>& );
    expr toZ3();
};

} /* namespace borealis */
#endif /* Z3CONTEXT_H_ */
