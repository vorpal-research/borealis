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
    currentPtr(1U),
    bounds_(),
    boundsFunction_(BoundsFunction::mkFunc(factory.unwrap(), "$$__gep_bound__$$")) {};

Z3::Bool ExecutionContext::toSMT() const {
    Z3::Bool axiom = factory.getTrue();
    for (auto bound: bounds_) {
        auto bConst = factory.getIntConst(bound.second);
        axiom = axiom && (boundsFunction_(bound.first) == bConst);
    }
    Z3::Bool res = factory.getTrue();
    return res.withAxiom(axiom);
}

std::ostream& operator<<(std::ostream& s, const ExecutionContext& ctx) {
    using std::endl;
    return s << "ctx state:" << endl
             << "< offset = " << ctx.currentPtr << " >" << endl
             << ctx.memArrays;
}

} // namespace z3_
} // namespace borealis
