/*
 * ExecutionContext.cpp
 *
 *  Created on: Aug 5, 2013
 *      Author: sam
 */

#include "SMT/MathSAT/ExecutionContext.h"

namespace borealis {
namespace mathsat_ {

ExecutionContext::ExecutionContext(ExprFactory& factory):
    factory(factory),
    currentPtr(1U) {};

MathSAT::Bool ExecutionContext::toSMT() const {
    auto res = factory.getTrue();
    return res;
}

std::ostream& operator<<(std::ostream& s, const ExecutionContext& ctx) {
    using std::endl;
    return s << "ctx state:" << endl
             << "< offset = " << ctx.currentPtr << " >" << endl
             << ctx.memArrays;
}

} // namespace mathsat_
} // namespace borealis
