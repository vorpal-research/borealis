/*
 * StorePredicate.cpp
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#include "StorePredicate.h"

#include "typeindex.hpp"

namespace borealis {

StorePredicate::StorePredicate(
		const llvm::Value* lhv,
		const llvm::Value* rhv,
		SlotTracker* st) :
				lhv(lhv),
                rhv(rhv),
				lhvs(st->getLocalName(lhv)),
				rhvs(st->getLocalName(rhv)) {
	this->asString = "*" + lhvs + "=" + rhvs;
}

Predicate::Key StorePredicate::getKey() const {
	return std::make_pair(borealis::type_id(*this), lhv);
}

Predicate::Dependee StorePredicate::getDependee() const {
    return std::make_pair(DependeeType::DEREF_VALUE, lhv);
}

Predicate::DependeeSet StorePredicate::getDependees() const {
    DependeeSet res = DependeeSet();
    res.insert(std::make_pair(DependeeType::VALUE, rhv));
    return res;
}

z3::expr StorePredicate::toZ3(z3::context& ctx) const {
	using namespace::z3;

    expr l = valueToExpr(ctx, *lhv, lhvs);
    expr r = valueToExpr(ctx, *rhv, rhvs);

    func_decl deref = ctx.function("*", l.get_sort(), r.get_sort());

	return deref(l) == r;
}

} /* namespace borealis */
