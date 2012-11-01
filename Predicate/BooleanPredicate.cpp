/*
 * BooleanPredicate.cpp
 *
 *  Created on: Sep 26, 2012
 *      Author: ice-phoenix
 */

#include "BooleanPredicate.h"

namespace borealis {

BooleanPredicate::BooleanPredicate(
        const llvm::Value* v,
        const bool b,
        SlotTracker* st) :
	        v(v),
	        b(b),
	        _v(st->getLocalName(v)),
	        _b(b ? "TRUE" : "FALSE") {
    this->asString = _v + "=" + _b;
}

BooleanPredicate::BooleanPredicate(
        const PredicateType type,
        const llvm::Value* v,
        const bool b,
        SlotTracker* st) : BooleanPredicate(v, b, st) {
    this->type = type;
}

Predicate::Key BooleanPredicate::getKey() const {
    return std::make_pair(borealis::type_id(*this), v);
}

Predicate::Dependee BooleanPredicate::getDependee() const {
    return std::make_pair(DependeeType::NONE, nullptr);
}

Predicate::DependeeSet BooleanPredicate::getDependees() const {
    DependeeSet res = DependeeSet();
    res.insert(std::make_pair(DependeeType::VALUE, v));
    return res;
}

z3::expr BooleanPredicate::toZ3(Z3ExprFactory& z3ef) const {
    using namespace::z3;

    expr var = z3ef.getBoolVar(_v);
    expr val = z3ef.getBoolConst(b);
    return var == val;
}

} /* namespace borealis */
