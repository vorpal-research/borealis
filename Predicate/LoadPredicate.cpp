/*
 * LoadPredicate.cpp
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#include "LoadPredicate.h"

namespace borealis {

LoadPredicate::LoadPredicate(
		const llvm::Value* lhv,
		const llvm::Value* rhv,
		SlotTracker* st) :
				lhv(lhv),
				rhv(rhv),
				lhvs(st->getLocalName(lhv)),
				rhvs("*" + st->getLocalName(rhv)) {
	this->asString = lhvs + "=" + rhvs;
}

Predicate::Key LoadPredicate::getKey() const {
	return std::make_pair(std::type_index(typeid(*this)).hash_code(), lhv);
}

z3::expr LoadPredicate::toZ3(z3::context& ctx) const {
	using namespace::z3;

	expr l = valueToExpr(ctx, *lhv, lhvs);
	expr r = derefValueToExpr(ctx, *rhv, rhvs);
	return l == r;
}

} /* namespace borealis */
