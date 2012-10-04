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

} /* namespace borealis */
