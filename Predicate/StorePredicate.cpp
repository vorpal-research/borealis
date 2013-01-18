/*
 * StorePredicate.cpp
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#include "StorePredicate.h"

#include "Term/ValueTerm.h"
#include "Logging/logger.hpp"

#include "Util/macros.h"

namespace borealis {

StorePredicate::StorePredicate(
        PredicateType type,
        Term::Ptr lhv,
        Term::Ptr rhv) :
            StorePredicate(lhv, rhv, nullptr, type) {}

StorePredicate::StorePredicate(
        Term::Ptr lhv,
        Term::Ptr rhv,
        SlotTracker* /* st */,
        PredicateType type) :
            Predicate(type_id(*this), type),
            lhv(lhv),
            rhv(rhv) {
    this->asString = "*" + this->lhv->getName() + "=" + this->rhv->getName();
}

logic::Bool StorePredicate::toZ3(Z3ExprFactory& z3ef, ExecutionContext* ctx) const {
    TRACE_FUNC;

    typedef Z3ExprFactory::Pointer Pointer;

    auto l = z3ef.getExprForTerm(*lhv);
    auto r = z3ef.getExprForTerm(*rhv);

    if (ctx) {
        auto lptr = l.to<Pointer>();

        if (lptr.empty()) {
            BYE_BYE(logic::Bool, "Store dealing with a non-pointer value");
        }
        ctx->writeExprToMemory(lptr.getUnsafe(), r);
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

#include "Util/unmacros.h"
