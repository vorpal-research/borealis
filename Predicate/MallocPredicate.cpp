/*
 * MallocPredicate.cpp
 *
 *  Created on: Nov 30, 2012
 *      Author: ice-phoenix
 */

#include "MallocPredicate.h"
#include "Logging/tracer.hpp"

namespace borealis {

MallocPredicate::MallocPredicate(
        Term::Ptr lhv) : Predicate(type_id(*this)),
            lhv(std::move(lhv)) {
    this->asString = this->lhv->getName() + "=malloc()";
}

MallocPredicate::MallocPredicate(
        Term::Ptr lhv,
        SlotTracker* /*st*/) : Predicate(type_id(*this)),
            lhv(std::move(lhv)) {
    this->asString = this->lhv->getName() + "=malloc()";
}

Predicate::Key MallocPredicate::getKey() const {
    return std::make_pair(type_id(*this), lhv->getId());
}

logic::Bool MallocPredicate::toZ3(Z3ExprFactory& z3ef, ExecutionContext* ctx) const {
    TRACE_FUNC;

    typedef Z3ExprFactory::Pointer Pointer;

    auto lhve = z3ef.getExprForTerm(*lhv, Pointer::bitsize);
    if (!lhve.is<Pointer>())
        return util::sayonara<logic::Bool>(__FILE__, __LINE__, __PRETTY_FUNCTION__,
                "Malloc predicate produces a non-pointer");

    Pointer lhvp = lhve.to<Pointer>().getUnsafe();

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
    size_t hash = 3;
    hash = 17 * hash + lhv->hashCode();
    return hash;
}

} /* namespace borealis */
