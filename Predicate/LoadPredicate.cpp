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

logic::Bool LoadPredicate::toZ3(Z3ExprFactory& z3ef, ExecutionContext* ctx) const {
    TRACE_FUNC;
    typedef Z3ExprFactory::Pointer Pointer;
    typedef Z3ExprFactory::Dynamic Dynamic;

    Dynamic l = z3ef.getExprForTerm(*lhv);
    Dynamic r = z3ef.getExprForTerm(*rhv);
    if(!r.is<Pointer>())
        return util::sayonara<logic::Bool>(__FILE__,__LINE__,__PRETTY_FUNCTION__,
                "Encountered load predicate with non-pointer right side");

    auto rp = r.to<Pointer>().getUnsafe();

    if (ctx) {
        return z3ef.if_(z3ef.isInvalidPtrExpr(rp))
                   .then_(z3ef.getFalse())
                   .else_(l == ctx->readExprFromMemory(rp, l.get_sort().bv_size()));
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
