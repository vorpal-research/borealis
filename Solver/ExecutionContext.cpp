/*
 * Z3Context.cpp
 *
 *  Created on: Nov 22, 2012
 *      Author: belyaev
 */

#include "ExecutionContext.h"
#include "Logging/tracer.hpp"

namespace borealis {

ExecutionContext::ExecutionContext(Z3ExprFactory& factory):
    factory(factory),
    memory(factory.getNoMemoryArray()) {};

logic::Bool ExecutionContext::toZ3() {
    TRACE_FUNC;

    if(allocated_pointers.empty() || allocated_pointers.size() == 1)
        return logic::Bool::mkConst(factory.unwrap(), true);

    return factory.getDistinct(allocated_pointers);
}


} // namespace borealis
