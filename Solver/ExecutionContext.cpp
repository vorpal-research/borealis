/*
 * Z3Context.cpp
 *
 *  Created on: Nov 22, 2012
 *      Author: belyaev
 */

#include "ExecutionContext.h"
#include "Logging/tracer.hpp"

namespace borealis {

void ExecutionContext::mutateMemory( const std::function<z3::func_decl(z3::func_decl)>& mutator ) {
    TRACE_FUNC;
    memory = mutator(memory);
}

ExecutionContext::ExecutionContext(Z3ExprFactory& factory):
    factory(factory),
    memory(factory.getNoMemoryArray()),
    axiom(factory.getNoMemoryArrayAxiom(memory)){};

z3::expr ExecutionContext::readExprFromMemory(z3::expr ix, size_t sz) {
    TRACE_FUNC;
    return factory.byteArrayExtract(memory, ix, sz);
}
void ExecutionContext::writeExprToMemory(z3::expr ix, z3::expr val) {
    TRACE_FUNC;

    auto sh = factory.byteArrayInsert(memory, ix, val);
    memory = sh.first;
    axiom = axiom && sh.second;
}

z3::expr ExecutionContext::toZ3() {
    TRACE_FUNC;

    if(allocated_pointers.empty() || allocated_pointers.size() == 1)
        return axiom;

    return axiom && factory.getDistinct(allocated_pointers);
}


} // namespace borealis
