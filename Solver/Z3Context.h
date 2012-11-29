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

class Z3Context {
    Z3ExprFactory& factory;
    Z3ExprFactory::z3ExprRef memory;

    std::vector<z3::expr> allocated_pointers;
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

    inline void registerDistinctPtr(z3::expr ptr) {
        allocated_pointers.push_back(ptr);
    }

    z3::expr toZ3() {
        z3::context& ctx = factory.unwrap();
        if(allocated_pointers.empty() || allocated_pointers.size() == 1)
            return factory.getBoolConst(true);

        std::vector<Z3_ast> cast;
        for(auto e: allocated_pointers) {
            cast.push_back(e);
        }

        return z3::to_expr(ctx, Z3_mk_distinct(ctx, cast.size(), &cast[0]));
    }

};

} /* namespace borealis */
#endif /* Z3CONTEXT_H_ */
