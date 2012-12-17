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

logic::Bool StorePredicate::toZ3(Z3ExprFactory& z3ef, ExecutionContext* ctx) const {
    TRACE_FUNC;

    auto l = z3ef.getExprForTerm(*lhv);
    auto r = z3ef.getExprForTerm(*rhv);

    if (ctx) {
        auto lptr = l.to<Z3ExprFactory::Pointer>();

        if(lptr.empty()) {
            return util::sayonara<logic::Bool>(__FILE__, __LINE__, __PRETTY_FUNCTION__,
                    "StorePredicate dealing with a non-pointer value");
        }
        ctx->writeExprToMemory(*lptr.get(), r);
    }
    return z3ef.getTrue();
}

bool StorePredicate::equals(const Predicate* other) const {
    if (other == nullptr) return false;
    if (this == other) return true;
    if (const StorePredicate* o = llvm::dyn_cast<StorePredicate>(other)) {
        return *this->lhv == *o->lhv &&
                *this->rhv == *o->rhv;
    } else {
        return false;
    }
}

size_t StorePredicate::hashCode() const {
    size_t hash = 3;
    hash = 17 * hash + lhv->hashCode();
    hash = 17 * hash + rhv->hashCode();
    return hash;
}

} /* namespace borealis */
