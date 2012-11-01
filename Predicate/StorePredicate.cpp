/*
 * StorePredicate.cpp
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#include "StorePredicate.h"

namespace borealis {

StorePredicate::StorePredicate(
        const llvm::Value* lhv,
        const llvm::Value* rhv,
        SlotTracker* st) :
				        lhv(lhv),
				        rhv(rhv),
				        _lhv(st->getLocalName(lhv)),
				        _rhv(st->getLocalName(rhv)) {
    this->asString = "*" + _lhv + "=" + _rhv;
}

Predicate::Key StorePredicate::getKey() const {
    return std::make_pair(borealis::type_id(*this), lhv);
}

Predicate::Dependee StorePredicate::getDependee() const {
    return std::make_pair(DependeeType::DEREF_VALUE, lhv);
}

Predicate::DependeeSet StorePredicate::getDependees() const {
    DependeeSet res = DependeeSet();
    res.insert(std::make_pair(DependeeType::VALUE, rhv));
    return res;
}

z3::expr StorePredicate::toZ3(Z3ExprFactory& z3ef) const {
    using namespace::z3;

    expr l = z3ef.getExprForValue(*lhv, _lhv);
    expr r = z3ef.getExprForValue(*rhv, _rhv);

    sort domain = l.get_sort();
    sort range = r.get_sort();
    func_decl deref = z3ef.getDerefFunction(domain, range);

    return deref(l) == r;
}

} /* namespace borealis */
