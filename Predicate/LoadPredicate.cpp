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
				rhvs(st->getLocalName(rhv)) {
	this->asString = lhvs + "=*(" + rhvs + ")";
}

Predicate::Key LoadPredicate::getKey() const {
	return std::make_pair(std::type_index(typeid(*this)).hash_code(), lhv);
}

Predicate::Dependee LoadPredicate::getDependee() const {
    return std::make_pair(DependeeType::VALUE, lhv);
}

Predicate::DependeeSet LoadPredicate::getDependees() const {
    DependeeSet res = DependeeSet();
    res.insert(std::make_pair(DependeeType::DEREF_VALUE, rhv));
    return res;
}

z3::expr LoadPredicate::toZ3(z3::context& ctx) const {
	using namespace::z3;

    expr l = valueToExpr(ctx, *lhv, lhvs);
    expr r = valueToExpr(ctx, *rhv, rhvs);

    func_decl deref = ctx.function("*", r.get_sort(), l.get_sort());

	return l == deref(r);
}

} /* namespace borealis */
