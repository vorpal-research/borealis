/*
 * EqualityPredicate.cpp
 *
 *  Created on: Oct 26, 2012
 *      Author: ice-phoenix
 */

#include "EqualityPredicate.h"

#include "Term/ValueTerm.h"

namespace borealis {

EqualityPredicate::EqualityPredicate(
        Term::Ptr lhv,
        Term::Ptr rhv) : Predicate(type_id(*this)),
            lhv(std::move(lhv)),
            rhv(std::move(rhv)) {
    this->asString = this->lhv->getName() + "=" + this->rhv->getName();
}

EqualityPredicate::EqualityPredicate(
        Term::Ptr lhv,
        Term::Ptr rhv,
        SlotTracker* /* st */) : Predicate(type_id(*this)),
            lhv(std::move(lhv)),
            rhv(std::move(rhv)) {
    this->asString = this->lhv->getName() + "=" + this->rhv->getName();
}

Predicate::Key EqualityPredicate::getKey() const {
    return std::make_pair(type_id(*this), lhv->getId());
}

Predicate::Dependee EqualityPredicate::getDependee() const {
    return std::make_pair(DependeeType::VALUE, lhv->getId());
}

Predicate::DependeeSet EqualityPredicate::getDependees() const {
    DependeeSet res = DependeeSet();
    res.insert(std::make_pair(DependeeType::VALUE, rhv->getId()));
    return res;
}

z3::expr EqualityPredicate::toZ3(Z3ExprFactory& z3ef, Z3Context*) const {
    using namespace::z3;

    TRACE_FUNC;

    expr l = z3ef.getExprForTerm(*lhv);
    expr r = z3ef.getExprForTerm(*rhv);

    return l == r;
}

} /* namespace borealis */
