/*
 * GEPPredicate.cpp
 *
 *  Created on: Oct 23, 2012
 *      Author: ice-phoenix
 */

#include "GEPPredicate.h"

#include "util.h"

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

    std::string a = _rhv;
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

    this->asString = _lhv + "=gep(" + a + ")";
}

Predicate::Key GEPPredicate::getKey() const {
    return std::make_pair(std::type_index(typeid(*this)).hash_code(), lhv);
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

    sort I = ctx.int_sort();
    func_decl gep = ctx.function("gep", I, I);

    expr l = valueToExpr(ctx, *lhv, _lhv);
    expr r = valueToExpr(ctx, *rhv, _rhv);

    for (const auto& s : shifts) {
        expr by = valueToExpr(ctx, *std::get<0>(s), std::get<1>(s));
        expr size = ctx.int_val((__uint64)std::get<2>(s));
        r = r + by * size;
    }

    return l == gep(r);
}

} /* namespace borealis */
