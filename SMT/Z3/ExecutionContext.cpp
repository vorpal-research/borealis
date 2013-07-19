/*
 * ExecutionContext.cpp
 *
 *  Created on: Nov 22, 2012
 *      Author: belyaev
 */

#include "SMT/Z3/ExecutionContext.h"

namespace borealis {
namespace z3_ {

ExecutionContext::ExecutionContext(ExprFactory& factory):
    factory(factory),
    currentPtr(1U) {};

Z3::Bool ExecutionContext::toSMT() const {
    return factory.getTrue();
}

std::ostream& operator<<(std::ostream& s, const ExecutionContext& ctx) {
    using std::endl;
    return s << "ctx state:" << endl
             << "< offset = " << ctx.currentPtr << " >" << endl
             << ctx.memArrays;
}

} // namespace z3_
} // namespace borealis
