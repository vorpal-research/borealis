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
        Term::Ptr numElements):
                Predicate(type_id(*this)),
                lhv(std::move(lhv)),
                numElements(std::move(numElements)) {
    this->asString = this->lhv->getName() + "=alloca(" + this->numElements->getName() + ")";
}

AllocaPredicate::AllocaPredicate(
        Term::Ptr lhv,
        Term::Ptr numElements,
        SlotTracker* /*st*/): Predicate(type_id(*this)),
                lhv(std::move(lhv)),
                numElements(std::move(numElements)) {
    this->asString = this->lhv->getName() + "=alloca(" + this->numElements->getName() + ")";
}

Predicate::Key AllocaPredicate::getKey() const {
    return std::make_pair(type_id(*this), lhv->getId());
}

Predicate::Dependee AllocaPredicate::getDependee() const {
    return std::make_pair(DependeeType::VALUE, lhv->getId());
}
Predicate::DependeeSet AllocaPredicate::getDependees() const {
    DependeeSet res = DependeeSet();
    res.insert(std::make_pair(DependeeType::VALUE, numElements->getId()));
    return res;
}

z3::expr AllocaPredicate::toZ3(Z3ExprFactory& z3ef, Z3Context* ctx) const {
    TRACE_FUNC;

    z3::expr lhve = z3ef.getExprForTerm(*lhv, z3ef.getPtrSort().bv_size());
    if(ctx) {
        ctx->registerDistinctPtr(lhve);
    }

    return !z3ef.isInvalidPtrExpr(lhve);
}

} /* namespace borealis */
