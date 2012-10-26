/*
 * ICmpPredicate.cpp
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#include "ICmpPredicate.h"

#include "util.h"

#include "typeindex.hpp"

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
				conds(conditionToString(cond)) {
	this->asString = lhvs + "=" + op1s + conds + op2s;
}

Predicate::Key ICmpPredicate::getKey() const {
	return std::make_pair(borealis::type_id(*this), lhv);
}

Predicate::Dependee ICmpPredicate::getDependee() const {
    return std::make_pair(DependeeType::VALUE, lhv);
}

Predicate::DependeeSet ICmpPredicate::getDependees() const {
    DependeeSet res = DependeeSet();
    res.insert(std::make_pair(DependeeType::VALUE, op1));
    res.insert(std::make_pair(DependeeType::VALUE, op2));
    return res;
}

z3::expr ICmpPredicate::toZ3(z3::context& ctx) const {
	using namespace::z3;

	expr l = ctx.bool_const(lhvs.c_str());
	expr o1 = valueToExpr(ctx, *op1, op1s);
	expr o2 = valueToExpr(ctx, *op2, op2s);

	ConditionType ct = conditionToType(cond);
	switch(ct) {
	case ConditionType::EQ: return l == (o1 == o2);
	case ConditionType::NEQ: return l == (o1 != o2);
	case ConditionType::LT: return l == (o1 < o2);
	case ConditionType::LTE: return l == (o1 <= o2);
	case ConditionType::GT: return l == (o1 > o2);
	case ConditionType::GTE: return l == (o1 >= o2);
	case ConditionType::TRUE: return l == ctx.bool_val(true);
	case ConditionType::FALSE: return l == ctx.bool_val(false);
	case ConditionType::WTF: return *((z3::expr*)0);
	}
}

} /* namespace borealis */
