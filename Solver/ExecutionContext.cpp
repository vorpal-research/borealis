/*
 * Z3Context.cpp
 *
 *  Created on: Nov 22, 2012
 *      Author: belyaev
 */

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

std::ostream& operator<<(std::ostream& s, const ExecutionContext& ctx) {
    using std::endl;
    return s << "Ctx state:" << endl
             << "< offset = " << ctx.currentPtr << " >" << endl
             << ctx.memory;
}

} // namespace borealis
