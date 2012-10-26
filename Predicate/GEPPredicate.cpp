/*
 * GEPPredicate.cpp
 *
 *  Created on: Oct 23, 2012
 *      Author: ice-phoenix
 */

#include "GEPPredicate.h"

#include "typeindex.hpp"

namespace borealis {

GEPPredicate::GEPPredicate(
        const llvm::Value* lhv,
        const llvm::Value* rhv,
        const std::vector< std::pair<const llvm::Value*, uint64_t> > shifts,
        SlotTracker* st) :
                lhv(lhv),
                rhv(rhv),
                _lhv(st->getLocalName(lhv)),
                _rhv(st->getLocalName(rhv)) {

    std::string a = "0";
    for (const auto& shift : shifts) {
        std::string by = st->getLocalName(shift.first);
        std::string size = util::toString(shift.second);
        this->shifts.push_back(
                std::make_tuple(
                        shift.first,
                        by,
                        shift.second)
        );
        a = a + "+" + by + "*" + size;
    }

    this->asString = _lhv + "=gep(" + _rhv + "," + a + ")";
}

Predicate::Key GEPPredicate::getKey() const {
    return std::make_pair(type_id(*this), lhv);
}

Predicate::Dependee GEPPredicate::getDependee() const {
    return std::make_pair(DependeeType::VALUE, lhv);
}

Predicate::DependeeSet GEPPredicate::getDependees() const {
    DependeeSet res = DependeeSet();
    // TODO: ...
    return res;
}

z3::expr GEPPredicate::toZ3(z3::context& ctx) const {
    using namespace::z3;

    expr l = valueToExpr(ctx, *lhv, _lhv);
    expr r = valueToExpr(ctx, *rhv, _rhv);

    sort domain[] = {r.get_sort(), r.get_sort()};
    func_decl gep = ctx.function("gep", 2, domain, l.get_sort());

    expr shift = ctx.int_val(0);
    for (const auto& s : shifts) {
        expr by = valueToExpr(ctx, *std::get<0>(s), std::get<1>(s));
        expr size = ctx.int_val((__uint64)std::get<2>(s));
        shift = shift + by * size;
    }

    return l == gep(r, shift);
}

} /* namespace borealis */
