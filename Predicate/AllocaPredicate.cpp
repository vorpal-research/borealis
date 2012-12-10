/*
 * AllocaPredicate.cpp
 *
 *  Created on: Nov 28, 2012
 *      Author: belyaev
 */

#include "AllocaPredicate.h"
#include "Logging/tracer.hpp"

namespace borealis {

AllocaPredicate::AllocaPredicate(
        Term::Ptr lhv,
        Term::Ptr numElements) : Predicate(type_id(*this)),
                lhv(std::move(lhv)),
                numElements(std::move(numElements)) {
    this->asString = this->lhv->getName() + "=alloca(" + this->numElements->getName() + ")";
}

AllocaPredicate::AllocaPredicate(
        Term::Ptr lhv,
        Term::Ptr numElements,
        SlotTracker* /*st*/) : Predicate(type_id(*this)),
                lhv(std::move(lhv)),
                numElements(std::move(numElements)) {
    this->asString = this->lhv->getName() + "=alloca(" + this->numElements->getName() + ")";
}

Predicate::Key AllocaPredicate::getKey() const {
    return std::make_pair(type_id(*this), lhv->getId());
}

z3::expr AllocaPredicate::toZ3(Z3ExprFactory& z3ef, ExecutionContext* ctx) const {
    TRACE_FUNC;

    z3::expr lhve = z3ef.getExprForTerm(*lhv, z3ef.getPtrSort().bv_size());
    if (ctx) {
        ctx->registerDistinctPtr(lhve);
    }

    return !z3ef.isInvalidPtrExpr(lhve);
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
    size_t hash = 3;
    hash = 17 * hash + lhv->hashCode();
    hash = 17 * hash + numElements->hashCode();
    return hash;
}

} /* namespace borealis */
