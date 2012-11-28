/*
 * Z3Context.h
 *
 *  Created on: Nov 22, 2012
 *      Author: belyaev
 */

#ifndef Z3CONTEXT_H_
#define Z3CONTEXT_H_

#include <z3/z3++.h>

#include "Solver/Z3ExprFactory.h"

namespace borealis {

class Z3Context {
    Z3ExprFactory& factory;
    Z3ExprFactory::z3ExprRef memory;
public:
    Z3Context(Z3ExprFactory& factory): factory(factory), memory(factory.getEmptyMemoryArray()){};

    z3::expr getCurrentMemoryContents() { return memory; }
    z3::expr readExprFromMemory(z3::expr ix, size_t sz) {
        return factory.byteArrayExtract(memory, ix, sz);
    }
    void writeExprToMemory(z3::expr ix, z3::expr val) {
        memory = factory.byteArrayInsert(memory, ix, val);
    }

    void mutateMemory( const std::function<z3::expr(z3::expr)>& );

};

} /* namespace borealis */
#endif /* Z3CONTEXT_H_ */
