/*
 * AllocaPredicate.cpp
 *
 *  Created on: Nov 28, 2012
 *      Author: belyaev
 */

#include "Predicate/AllocaPredicate.h"
#include "Util/macros.h"

namespace borealis {

AllocaPredicate::AllocaPredicate(
        Term::Ptr lhv,
        Term::Ptr numElements,
        PredicateType type) :
            Predicate(type_id(*this), type),
            lhv(lhv),
            numElements(numElements) {
    this->asString = this->lhv->getName() + "=alloca(" + this->numElements->getName() + ")";
}

logic::Bool AllocaPredicate::toZ3(Z3ExprFactory& z3ef, ExecutionContext* ctx) const {
    TRACE_FUNC;

    typedef Z3ExprFactory::Pointer Pointer;

    auto lhve = lhv->toZ3(z3ef, ctx);

    ASSERT(lhve.is<Pointer>(),
           "Encountered alloca with non-Pointer left side");

    auto lhvp = lhve.to<Pointer>().getUnsafe();
    if (ctx) {
        ctx->registerDistinctPtr(lhvp);
    }

    return !z3ef.isInvalidPtrExpr(lhvp);
}

bool AllocaPredicate::equals(const Predicate* other) const {
    if (other == nullptr) return false;
    if (this == other) return true;
    if (const AllocaPredicate* o = llvm::dyn_cast<AllocaPredicate>(other)) {
        return *this->lhv == *o->lhv &&
                *this->numElements == *o->numElements;
    } else {
        return false;
    }
}

size_t AllocaPredicate::hashCode() const {
    return util::hash::hasher<3, 17>()(type, lhv, numElements);
}

} /* namespace borealis */

#include "Util/unmacros.h"
