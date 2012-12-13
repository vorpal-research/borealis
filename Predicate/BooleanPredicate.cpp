/*
 * BooleanPredicate.cpp
 *
 *  Created on: Sep 26, 2012
 *      Author: ice-phoenix
 */

#include "BooleanPredicate.h"

#include "Term/ConstTerm.h"

namespace borealis {

BooleanPredicate::BooleanPredicate(
        Term::Ptr v,
        Term::Ptr b) : Predicate(type_id(*this)),
            v(std::move(v)),
            b(std::move(b)) {
    this->asString = this->v->getName() + "=" + this->b->getName();
}

BooleanPredicate::BooleanPredicate(
        Term::Ptr v,
        bool b,
        SlotTracker* /* st */) : Predicate(type_id(*this)),
	        v(std::move(v)),
	        b(new ConstTerm(llvm::ValueType::BOOL_CONST, b ? "TRUE" : "FALSE")) {
    this->asString = this->v->getName() + "=" + this->b->getName();
}

BooleanPredicate::BooleanPredicate(
        PredicateType type,
        Term::Ptr v,
        bool b,
        SlotTracker* /* st */) : Predicate(type_id(*this), type),
            v(std::move(v)),
            b(new ConstTerm(llvm::ValueType::BOOL_CONST, b ? "TRUE" : "FALSE")) {
}

Predicate::Key BooleanPredicate::getKey() const {
    return std::make_pair(borealis::type_id(*this), v->getId());
}

logic::Bool BooleanPredicate::toZ3(Z3ExprFactory& z3ef, ExecutionContext*) const {
    TRACE_FUNC;

    auto var = z3ef.getExprForTerm(*v);
    auto val = z3ef.getExprForTerm(*b);
    return var == val;
}

bool BooleanPredicate::equals(const Predicate* other) const {
    if (other == nullptr) return false;
    if (this == other) return true;
    if (const BooleanPredicate* o = llvm::dyn_cast<BooleanPredicate>(other)) {
        return *this->v == *o->v &&
                *this->b == *o->b;
    } else {
        return false;
    }
}

size_t BooleanPredicate::hashCode() const {
    size_t hash = 3;
    hash = 17 * hash + v->hashCode();
    hash = 17 * hash + b->hashCode();
    return hash;
}

} /* namespace borealis */
