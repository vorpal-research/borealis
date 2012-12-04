/*
 * LoadPredicate.cpp
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#include "LoadPredicate.h"

#include "Term/ValueTerm.h"

namespace borealis {

LoadPredicate::LoadPredicate(
        Term::Ptr lhv,
        Term::Ptr rhv) : Predicate(type_id(*this)),
            lhv(std::move(lhv)),
            rhv(std::move(rhv)) {
    this->asString = this->lhv->getName() + "=*" + this->rhv->getName();
}

LoadPredicate::LoadPredicate(
        Term::Ptr lhv,
        Term::Ptr rhv,
        SlotTracker* /* st */) : Predicate(type_id(*this)),
            lhv(std::move(lhv)),
            rhv(std::move(rhv)) {
    this->asString = this->lhv->getName() + "=*" + this->rhv->getName();
}

Predicate::Key LoadPredicate::getKey() const {
    return std::make_pair(type_id(*this), lhv->getId());
}

Predicate::Dependee LoadPredicate::getDependee() const {
    return std::make_pair(DependeeType::VALUE, lhv->getId());
}

Predicate::DependeeSet LoadPredicate::getDependees() const {
    DependeeSet res = DependeeSet();
    res.insert(std::make_pair(DependeeType::DEREF_VALUE, rhv->getId()));
    return res;
}

z3::expr LoadPredicate::toZ3(Z3ExprFactory& z3ef, ExecutionContext* z3ctx) const {
    using namespace::z3;

    TRACE_FUNC;

    expr l = z3ef.getExprForTerm(*lhv);
    expr r = z3ef.getExprForTerm(*rhv);


    if(z3ctx) {
        return z3ef.if_(z3ef.isInvalidPtrExpr(r)).
                       then_(z3ef.getBoolConst(false)).
                       else_(l == z3ctx->readExprFromMemory(r, l.get_sort().bv_size()/8));
    }

    return z3ef.getBoolConst(true);
}

} /* namespace borealis */
