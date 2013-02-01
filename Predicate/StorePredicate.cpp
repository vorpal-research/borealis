/*
 * StorePredicate.cpp
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#include "Predicate/StorePredicate.h"
#include "Term/ValueTerm.h"
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
    return util::hash::hasher<3, 17>()(lhv, rhv);
}

} /* namespace borealis */

#include "Util/unmacros.h"
