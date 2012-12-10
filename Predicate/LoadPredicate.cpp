/*
 * LoadPredicate.cpp
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#include "LoadPredicate.h"

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

z3::expr LoadPredicate::toZ3(Z3ExprFactory& z3ef, ExecutionContext* ctx) const {
    TRACE_FUNC;

    z3::expr l = z3ef.getExprForTerm(*lhv);
    z3::expr r = z3ef.getExprForTerm(*rhv);


    if (ctx) {
        return z3ef.if_(z3ef.isInvalidPtrExpr(r))
                   .then_(z3ef.getFalse())
                   .else_(l == ctx->readExprFromMemory(r, l.get_sort().bv_size()/8));
    }

    return z3ef.getTrue();
}

bool LoadPredicate::equals(const Predicate* other) const {
    if (other == nullptr) return false;
    if (this == other) return true;
    if (const LoadPredicate* o = llvm::dyn_cast<LoadPredicate>(other)) {
        return *this->lhv == *o->lhv &&
                *this->rhv == *o->rhv;
    } else {
        return false;
    }
}

size_t LoadPredicate::hashCode() const {
    size_t hash = 3;
    hash = 17 * hash + lhv->hashCode();
    hash = 17 * hash + rhv->hashCode();
    return hash;
}

} /* namespace borealis */
