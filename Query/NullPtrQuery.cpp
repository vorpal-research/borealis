/*
 * NullPtrQuery.cpp
 *
 *  Created on: Oct 11, 2012
 *      Author: ice-phoenix
 */

#include "NullPtrQuery.h"

namespace borealis {

NullPtrQuery::NullPtrQuery(const llvm::Value* ptr, SlotTracker* st) :
        ptr(ptr),
        ptrs(st->getLocalName(ptr)) {
}

z3::expr NullPtrQuery::toZ3(z3::context& ctx) const {
    using namespace::z3;

    expr p = ctx.int_const(ptrs.c_str());
    expr zero = ctx.int_val(0);
    return p == zero;
}

NullPtrQuery::~NullPtrQuery() {
    // TODO
}

} /* namespace borealis */
