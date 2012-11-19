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
        const llvm::Value* lhv,
        const llvm::Value* rhv,
        SlotTracker* st) :
                    lhv(new ValueTerm(lhv, st)),
                    rhv(new ValueTerm(rhv, st)) {
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

z3::expr EqualityPredicate::toZ3(Z3ExprFactory& z3ef) const {
    using namespace::z3;

    expr l = z3ef.getExprForTerm(*lhv);
    expr r = z3ef.getExprForTerm(*rhv);

    return l == r;
}

} /* namespace borealis */
