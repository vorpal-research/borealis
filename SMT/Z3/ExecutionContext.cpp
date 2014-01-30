/*
 * ExecutionContext.cpp
 *
 *  Created on: Nov 22, 2012
 *      Author: belyaev
 */

#include "SMT/Z3/ExecutionContext.h"

namespace borealis {
namespace z3_ {

ExecutionContext::ExecutionContext(ExprFactory& factory, unsigned long long localMemory):
    factory(factory), globalPtr(1ULL), localPtr(localMemory) {
    initGepBounds();
};

Z3::Bool ExecutionContext::toSMT() const {
    return factory.getTrue();
}

std::ostream& operator<<(std::ostream& s, const ExecutionContext& ctx) {
    using std::endl;
    return s << "ctx state:" << endl
             << "< global offset = " << ctx.globalPtr << " >" << endl
             << "< local offset = " << ctx.localPtr << " >" << endl
             << ctx.memArrays;
}

std::string ExecutionContext::MEMORY_ID = "$$__memory__$$";
std::string ExecutionContext::GEP_BOUNDS_ID = "$$__gep_bound__$$";

} // namespace z3_
} // namespace borealis
