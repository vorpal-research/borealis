/*
 * NullPtrQuery.cpp
 *
 *  Created on: Oct 11, 2012
 *      Author: ice-phoenix
 */

#include "NullPtrQuery.h"

namespace borealis {

NullPtrQuery::NullPtrQuery(const llvm::Value* ptr, SlotTracker* st) :
        ptr(ptr), _ptr(st->getLocalName(ptr)) {
}

z3::expr NullPtrQuery::toZ3(Z3ExprFactory& z3ef) const {
    using namespace::z3;

    expr p = z3ef.getPtr(_ptr);
    expr null = z3ef.getNullPtr();
    return p == null;
}

std::string NullPtrQuery::toString() const {
    return _ptr + "=<null>";
}

NullPtrQuery::~NullPtrQuery() {}

} // namespace borealis
