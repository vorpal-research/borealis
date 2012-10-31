/*
 * ICmpPredicate.cpp
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#include "ICmpPredicate.h"

namespace borealis {

using util::sayonara;

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
				_lhv(st->getLocalName(lhv)),
				_op1(st->getLocalName(op1)),
				_op2(st->getLocalName(op2)),
				_cond(conditionString(cond)) {
	this->asString = _lhv + "=" + _op1 + _cond + _op2;
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

z3::expr ICmpPredicate::toZ3(Z3ExprFactory& z3ef) const {
	using namespace::z3;

	expr l = z3ef.getBoolVar(_lhv);
	expr o1 = z3ef.getExprForValue(*op1, _op1);
	expr o2 = z3ef.getExprForValue(*op2, _op2);

	ConditionType ct = conditionType(cond);
	switch(ct) {
	case ConditionType::EQ: return l == (o1 == o2);
	case ConditionType::NEQ: return l == (o1 != o2);
	case ConditionType::LT: return l == (o1 < o2);
	case ConditionType::LTE: return l == (o1 <= o2);
	case ConditionType::GT: return l == (o1 > o2);
	case ConditionType::GTE: return l == (o1 >= o2);
	case ConditionType::TRUE: return l == z3ef.getBoolConst(true);
	case ConditionType::FALSE: return l == z3ef.getBoolConst(false);
	case ConditionType::WTF: return sayonara<z3::expr>(__FILE__, __LINE__,
	        "<ICmpPredicate> Unknown condition type in Z3 conversion");
	}
}

} /* namespace borealis */
