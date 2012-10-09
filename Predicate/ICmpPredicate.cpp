/*
 * ICmpPredicate.cpp
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#include "ICmpPredicate.h"

#include "../util.h"

namespace borealis {

ICmpPredicate::ICmpPredicate(
		const Value* lhv,
		const Value* op1,
		const Value* op2,
		const int cond,
		SlotTracker* st) :
				lhv(lhv),
				op1(op1),
				op2(op2),
				cond(cond),
				lhvs(st->getLocalName(lhv)),
				op1s(st->getLocalName(op1)),
				op2s(st->getLocalName(op2)),
				conds(conditionToString(cond)),
				asString(lhvs + "=" + op1s + conds + op2s){
}

std::string ICmpPredicate::toString() const {
	return asString;
}

Predicate::Key ICmpPredicate::getKey() const {
	return std::make_pair(std::type_index(typeid(*this)).hash_code(), lhv);
}

z3::expr ICmpPredicate::toZ3(z3::context& ctx) const {
	using namespace::z3;

	expr l = ctx.bool_const(lhvs.c_str());
	expr o1 = valueToExpr(ctx, *op1, op1s);
	expr o2 = valueToExpr(ctx, *op2, op2s);

	ConditionType ct = conditionToType(cond);
	switch(ct) {
	case EQ: return l == (o1 == o2);
	case NEQ: return l == (o1 != o2);
	case LT: return l == (o1 < o2);
	case LTE: return l == (o1 <= o2);
	case GT: return l == (o1 > o2);
	case GTE: return l == (o1 >= o2);
	case TRUE: return l == ctx.bool_val(true);
	case FALSE: return l == ctx.bool_val(false);
	case WTF: return *((z3::expr*)0);
	}
}

} /* namespace borealis */
