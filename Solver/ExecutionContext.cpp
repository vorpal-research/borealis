/*
 * Z3Context.cpp
 *
 *  Created on: Nov 22, 2012
 *      Author: belyaev
 */

#include "Logging/tracer.hpp"
#include "Solver/ExecutionContext.h"

namespace borealis {

ExecutionContext::ExecutionContext(Z3ExprFactory& factory):
    factory(factory),
    memory(factory.getNoMemoryArray()) {};

logic::Bool ExecutionContext::toZ3() {
    TRACE_FUNC;

    if (allocated_pointers.size() < 2)
        return factory.getTrue();

    return factory.getDistinct(allocated_pointers);
}

} // namespace borealis
