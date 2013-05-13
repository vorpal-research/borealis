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
    memory(factory.getNoMemoryArray()),
    currentPtr(1U) {};

ExecutionContext::Bool ExecutionContext::toZ3() {
    TRACE_FUNC;

    return factory.getTrue();
}

} // namespace borealis
