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
    factory(factory), globalPtr(1ULL), localPtr(localMemory) {};

Z3::Bool ExecutionContext::toSMT() const {
    auto res = factory.getTrue();
    return res;
}

std::ostream& operator<<(std::ostream& s, const ExecutionContext& ctx) {
    using std::endl;
    return s << "ctx state:" << endl
             << "< global offset = " << ctx.globalPtr << " >" << endl
             << "< local offset = " << ctx.localPtr << " >" << endl
             << ctx.memArrays;
}

} // namespace z3_
} // namespace borealis
