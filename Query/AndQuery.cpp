/*
 * AndQuery.cpp
 *
 *  Created on: Nov 6, 2012
 *      Author: ice-phoenix
 */

#include "AndQuery.h"

namespace borealis {

using borealis::util::view;

AndQuery::AndQuery(std::initializer_list<Query*> qs) : qs(qs) {}

logic::Bool AndQuery::toZ3(Z3ExprFactory& z3ef) const {
    using namespace z3;
    using logic::Bool;

    Bool e = Bool::mkConst(z3ef.unwrap(), true);
    if (!qs.empty()) {
        auto it = qs.begin();
        e = (*it++)->toZ3(z3ef);
        for (auto& q : view(it, qs.end())) {
            e = e && q->toZ3(z3ef);
        }
    }
    return e;
}

std::string AndQuery::toString() const {
    std::string res = "AND(";
    if (!qs.empty()) {
        auto it = qs.begin();
        res += (*it++)->toString();
        for (auto& q : view(it, qs.end())) {
            res += ",";
            res += q->toString();
        }
    }
    res += ")";
    return res;
}

AndQuery::~AndQuery() {}

} /* namespace borealis */
