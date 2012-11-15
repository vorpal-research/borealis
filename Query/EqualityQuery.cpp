/*
 * EqualityQuery.cpp
 *
 *  Created on: Nov 6, 2012
 *      Author: ice-phoenix
 */

#include "EqualityQuery.h"

namespace borealis {

EqualityQuery::EqualityQuery(const llvm::Value* v1,
        const llvm::Value* v2,
        SlotTracker* st) :
                v1(v1),
                v2(v2),
                _v1(st->getLocalName(v1)),
                _v2(st->getLocalName(v2)) {}

z3::expr EqualityQuery::toZ3(Z3ExprFactory& z3ef) const {
    using namespace z3;

    expr l = z3ef.getExprForValue(*v1, _v1);
    expr r = z3ef.getExprForValue(*v2, _v2);
    return l == r;
}

std::string EqualityQuery::toString() const {
    return _v1 + "=" + _v2;
}

EqualityQuery::~EqualityQuery() {}

} /* namespace borealis */