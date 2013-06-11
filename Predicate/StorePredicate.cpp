/*
 * StorePredicate.cpp
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#include "Predicate/StorePredicate.h"

#include "Util/macros.h"

namespace borealis {

StorePredicate::StorePredicate(
        Term::Ptr lhv,
        Term::Ptr rhv,
        PredicateType type) :
            Predicate(type_id(*this), type),
            lhv(lhv),
            rhv(rhv) {
    this->asString = "*" + this->lhv->getName() + "=" + this->rhv->getName();
}

logic::Bool StorePredicate::toZ3(Z3ExprFactory& z3ef, ExecutionContext* ctx) const {
    TRACE_FUNC;

    typedef Z3ExprFactory::Pointer Pointer;

    ASSERTC(ctx != nullptr);

    auto l = lhv->toZ3(z3ef, ctx);
    auto r = rhv->toZ3(z3ef, ctx);

    ASSERT(l.is<Pointer>(),
           "Store dealing with a non-pointer value");

    ctx->writeExprToMemory(l.to<Pointer>().getUnsafe(), r);

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
    return util::hash::defaultHasher()(type, lhv, rhv);
}

} /* namespace borealis */

#include "Util/unmacros.h"
