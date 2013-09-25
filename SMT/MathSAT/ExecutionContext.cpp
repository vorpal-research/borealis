/*
 * ExecutionContext.cpp
 *
 *  Created on: Aug 5, 2013
 *      Author: sam
 */

#include "SMT/MathSAT/ExecutionContext.h"

namespace borealis {
namespace mathsat_ {

ExecutionContext::ExecutionContext(ExprFactory& factory, unsigned long long memoryStart):
    factory(factory), currentPtr(memoryStart) {};

MathSAT::Bool ExecutionContext::toSMT() const {
    return factory.getTrue();
}

std::ostream& operator<<(std::ostream& s, const ExecutionContext& ctx) {
    using std::endl;
    return s << "ctx state:" << endl
             << "< offset = " << ctx.currentPtr << " >" << endl
             << ctx.memArrays;
}

} // namespace mathsat_
} // namespace borealis
