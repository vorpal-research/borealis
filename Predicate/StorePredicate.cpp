/*
 * StorePredicate.cpp
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#include "StorePredicate.h"

#include "Term/ValueTerm.h"
#include "Logging/logger.hpp"

namespace borealis {

StorePredicate::StorePredicate(
        Term::Ptr lhv,
        Term::Ptr rhv) : Predicate(type_id(*this)),
            lhv(std::move(lhv)),
            rhv(std::move(rhv)) {
    this->asString = "*" + this->lhv->getName() + "=" + this->rhv->getName();
}

StorePredicate::StorePredicate(
        Term::Ptr lhv,
        Term::Ptr rhv,
        SlotTracker* /* st */) : Predicate(type_id(*this)),
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

z3::expr StorePredicate::toZ3(Z3ExprFactory& z3ef, Z3Context* z3ctx) const {
    using namespace::z3;

    TRACE_FUNC;

    expr l = z3ef.getExprForTerm(*lhv);
    expr r = z3ef.getExprForTerm(*rhv);

    if(z3ctx) {
        z3ctx->mutateMemory([&](z3::expr mem){
            return z3ef.if_(z3ef.isInvalidPtrExpr(l)).
                            then_(mem).
                            else_(z3ef.byteArrayInsert(mem, l, r));
        });
        return z3ef.if_(z3ef.isInvalidPtrExpr(l)).
                        then_(z3ef.getBoolConst(false)).
                        else_(z3ef.getBoolConst(true));
    }
    return z3ef.getBoolConst(true);
}

} /* namespace borealis */
