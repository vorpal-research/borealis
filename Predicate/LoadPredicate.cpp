/*
 * LoadPredicate.cpp
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#include "LoadPredicate.h"

namespace borealis {

LoadPredicate::LoadPredicate(
        const llvm::Value* lhv,
        const llvm::Value* rhv,
        SlotTracker* st) :
				        lhv(lhv),
				        rhv(rhv),
				        _lhv(st->getLocalName(lhv)),
				        _rhv(st->getLocalName(rhv)) {
    this->asString = _lhv + "=*" + _rhv;
}

Predicate::Key LoadPredicate::getKey() const {
    return std::make_pair(type_id(*this), lhv);
}

Predicate::Dependee LoadPredicate::getDependee() const {
    return std::make_pair(DependeeType::VALUE, lhv);
}

Predicate::DependeeSet LoadPredicate::getDependees() const {
    DependeeSet res = DependeeSet();
    res.insert(std::make_pair(DependeeType::DEREF_VALUE, rhv));
    return res;
}

z3::expr LoadPredicate::toZ3(Z3ExprFactory& z3ef) const {
    using namespace::z3;

    expr l = z3ef.getExprForValue(*lhv, _lhv);
    expr r = z3ef.getExprForValue(*rhv, _rhv);

    sort domain = r.get_sort();
    sort range = l.get_sort();
    func_decl deref = z3ef.getDerefFunction(domain, range);

    return l == deref(r);
}

} /* namespace borealis */
