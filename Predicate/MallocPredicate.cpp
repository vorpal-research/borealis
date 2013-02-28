/*
 * MallocPredicate.cpp
 *
 *  Created on: Nov 30, 2012
 *      Author: ice-phoenix
 */

#include "Predicate/MallocPredicate.h"
#include "Util/macros.h"

namespace borealis {

MallocPredicate::MallocPredicate(
        Term::Ptr lhv,
        PredicateType type) :
            Predicate(type_id(*this), type),
            lhv(lhv) {
    this->asString = this->lhv->getName() + "=malloc()";
}

logic::Bool MallocPredicate::toZ3(Z3ExprFactory& z3ef, ExecutionContext* ctx) const {
    TRACE_FUNC;

    typedef Z3ExprFactory::Pointer Pointer;

    auto lhve = lhv->toZ3(z3ef, ctx);

    ASSERT(lhve.is<Pointer>(),
           "Malloc produces a non-pointer");

    auto lhvp = lhve.to<Pointer>().getUnsafe();
    if (ctx) {
        ctx->registerDistinctPtr(lhvp);
    }

    return z3ef.getTrue();
}

bool MallocPredicate::equals(const Predicate* other) const {
    if (other == nullptr) return false;
    if (this == other) return true;
    if (const MallocPredicate* o = llvm::dyn_cast<MallocPredicate>(other)) {
        return *this->lhv == *o->lhv;
    } else {
        return false;
    }
}

size_t MallocPredicate::hashCode() const {
    return util::hash::hasher<3, 17>()(type, lhv);
}

} /* namespace borealis */

#include "Util/unmacros.h"
