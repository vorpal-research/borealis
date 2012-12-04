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

Predicate::Dependee MallocPredicate::getDependee() const {
    return std::make_pair(DependeeType::VALUE, lhv->getId());
}

Predicate::DependeeSet MallocPredicate::getDependees() const {
    DependeeSet res = DependeeSet();
    // FIXME akhin
    return res;
}

z3::expr MallocPredicate::toZ3(Z3ExprFactory& z3ef, ExecutionContext* ctx) const {
    TRACE_FUNC;

    z3::expr lhve = z3ef.getExprForTerm(*lhv, z3ef.getPtrSort().bv_size());
    if(ctx) {
        ctx->registerDistinctPtr(lhve);
    }

    return z3ef.getBoolConst(true);
}

} /* namespace borealis */
