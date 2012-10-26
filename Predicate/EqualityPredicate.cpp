/*
 * EqualityPredicate.cpp
 *
 *  Created on: Oct 26, 2012
 *      Author: ice-phoenix
 */

#include "EqualityPredicate.h"

#include "typeindex.hpp"

namespace borealis {

EqualityPredicate::EqualityPredicate(
        const llvm::Value* lhv,
        const llvm::Value* rhv,
        SlotTracker* st) :
            lhv(lhv),
            rhv(rhv),
            _lhv(st->getLocalName(lhv)),
            _rhv(st->getLocalName(rhv)) {
    this->asString = _lhv + "=" + _rhv;
}

Predicate::Key EqualityPredicate::getKey() const {
    return std::make_pair(type_id(*this), lhv);
}

Predicate::Dependee EqualityPredicate::getDependee() const {
    return std::make_pair(DependeeType::VALUE, lhv);
}

Predicate::DependeeSet EqualityPredicate::getDependees() const {
    DependeeSet res = DependeeSet();
    res.insert(std::make_pair(DependeeType::VALUE, rhv));
    return res;
}

z3::expr EqualityPredicate::toZ3(z3::context& ctx) const {
    using namespace::z3;

    expr l = valueToExpr(ctx, *lhv, _lhv);
    expr r = valueToExpr(ctx, *rhv, _rhv);

    return l == r;
}

} /* namespace borealis */
