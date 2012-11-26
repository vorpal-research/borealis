/*
 * BooleanPredicate.cpp
 *
 *  Created on: Sep 26, 2012
 *      Author: ice-phoenix
 */

#include "BooleanPredicate.h"

#include "Term/ConstTerm.h"
#include "Term/ValueTerm.h"

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
        SlotTracker* st) : BooleanPredicate(std::move(v), b, st) {
    this->type = type;
}

Predicate::Key BooleanPredicate::getKey() const {
    return std::make_pair(borealis::type_id(*this), v->getId());
}

Predicate::Dependee BooleanPredicate::getDependee() const {
    return std::make_pair(DependeeType::NONE, 0);
}

Predicate::DependeeSet BooleanPredicate::getDependees() const {
    DependeeSet res = DependeeSet();
    res.insert(std::make_pair(DependeeType::VALUE, v->getId()));
    return res;
}

z3::expr BooleanPredicate::toZ3(Z3ExprFactory& z3ef, Z3Context*) const {
    using namespace::z3;

    TRACE_FUNC;

    expr var = z3ef.getExprForTerm(*v);
    expr val = z3ef.getExprForTerm(*b);
    return var == val;
}

} /* namespace borealis */
