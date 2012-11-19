/*
 * StorePredicate.cpp
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#include "StorePredicate.h"

#include "Term/ValueTerm.h"

namespace borealis {

StorePredicate::StorePredicate(
        Term::Ptr lhv,
        Term::Ptr rhv,
        SlotTracker* st) :
				        lhv(std::move(lhv)),
				        rhv(std::move(rhv)) {
    this->asString = "*" + this->lhv->getName() + "=" + this->rhv->getName();
}

Predicate::Key StorePredicate::getKey() const {
    return std::make_pair(borealis::type_id(*this), lhv->getId());
}

Predicate::Dependee StorePredicate::getDependee() const {
    return std::make_pair(DependeeType::DEREF_VALUE, lhv->getId());
}

Predicate::DependeeSet StorePredicate::getDependees() const {
    DependeeSet res = DependeeSet();
    res.insert(std::make_pair(DependeeType::VALUE, rhv->getId()));
    return res;
}

z3::expr StorePredicate::toZ3(Z3ExprFactory& z3ef) const {
    using namespace::z3;

    expr l = z3ef.getExprForTerm(*lhv);
    expr r = z3ef.getExprForTerm(*rhv);

    sort domain = l.get_sort();
    sort range = r.get_sort();
    func_decl deref = z3ef.getDerefFunction(domain, range);

    return deref(l) == r;
}

} /* namespace borealis */
