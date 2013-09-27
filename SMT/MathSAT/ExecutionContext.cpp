/*
 * ExecutionContext.cpp
 *
 *  Created on: Aug 5, 2013
 *      Author: sam
 */

#include "SMT/MathSAT/ExecutionContext.h"

namespace borealis {
namespace mathsat_ {

ExecutionContext::ExecutionContext(ExprFactory& factory, unsigned long long localMemory):
    factory(factory), globalPtr(1ULL), localPtr(localMemory) {};

MathSAT::Bool ExecutionContext::toSMT() const {
    return factory.getTrue();
}

std::ostream& operator<<(std::ostream& s, const ExecutionContext& ctx) {
    using std::endl;
    return s << "ctx state:" << endl
             << "< global offset = " << ctx.globalPtr << " >" << endl
             << "< local offset = " << ctx.localPtr << " >" << endl
             << ctx.memArrays;
}

} // namespace mathsat_
} // namespace borealis
