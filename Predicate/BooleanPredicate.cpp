/*
 * BooleanPredicate.cpp
 *
 *  Created on: Sep 26, 2012
 *      Author: ice-phoenix
 */

#include "BooleanPredicate.h"

#include "typeindex.hpp"

namespace borealis {

BooleanPredicate::BooleanPredicate(
		const llvm::Value* v,
		const bool b,
		SlotTracker* st) :
	v(v),
	vs(st->getLocalName(v)),
	b(b),
	bs(b ? "TRUE" : "FALSE") {
	this->asString = vs + "=" + bs;
}

BooleanPredicate::BooleanPredicate(
		const PredicateType type,
		const llvm::Value* v,
		const bool b,
		SlotTracker* st) :
		        BooleanPredicate(v, b, st) {
	this-> type = type;
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

z3::expr BooleanPredicate::toZ3(z3::context& ctx) const {
	using namespace::z3;

	expr var = ctx.bool_const(vs.c_str());
	expr val = ctx.bool_val(b);
	return var == val;
}

} /* namespace borealis */
