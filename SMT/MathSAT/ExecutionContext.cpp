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
    factory(factory), currentPtr(1U), bounds_(),
    boundsFunction_(BoundsFunction::mkFunc(factory.unwrap(), "$$__gep_bound__$$")) {};

MathSAT::Bool ExecutionContext::toSMT() const {
    MathSAT::Bool axiom = factory.getTrue();
    for (auto bound: bounds_) {
        auto bConst = factory.getIntConst(bound.second);
        axiom = axiom && (boundsFunction_(bound.first) == bConst);
    }
    MathSAT::Bool res = factory.getTrue();
    return res.withAxiom(axiom);
}

std::ostream& operator<<(std::ostream& s, const ExecutionContext& ctx) {
    using std::endl;
    return s << "ctx state:" << endl
             << "< offset = " << ctx.currentPtr << " >" << endl
             << ctx.memArrays;
}

} // namespace mathsat_
} // namespace borealis
