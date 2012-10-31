/*
 * util.cpp
 *
 *  Created on: Oct 12, 2012
 *      Author: ice-phoenix
 */

#include "Solver/util.h"
#include "util.h"
#include "Z3ExprFactory.h"

namespace borealis {

using util::streams::endl;
using util::toString;

void addAxiomsForGEP(
        z3::func_decl& gep,
        z3::context& ctx,
        z3::solver& s) {
    using namespace::z3;

    Z3ExprFactory z3ef(ctx);

    auto ptr = to_expr(ctx, Z3_mk_bound(ctx, 0, ctx.bv_sort(32)));
    auto freevar = to_expr(ctx, Z3_mk_bound(ctx, 1, ctx.int_sort()));
    auto null = z3ef.getNullPtr();

    auto body = implies(
            ptr != null,
            gep(ptr, freevar) != null);

    Z3_sort sort_array[] = {Z3_sort(ctx.int_sort()), Z3_sort(ctx.bv_sort(32))};
    Z3_symbol name_array[] = {Z3_symbol(ctx.str_symbol("freevar")), Z3_symbol(ctx.str_symbol("ptr"))};

    auto axiom = to_expr(
            ctx,
            Z3_mk_forall(
                    ctx,
                    0,
                    0,
                    nullptr,
                    2,
                    sort_array,
                    name_array,
                    body));

    s.add(axiom);
}



z3::check_result check(
        const z3::expr& assertion,
        const std::vector<z3::expr>& state,
        z3::context& ctx) {
    using namespace::z3;

    solver s(ctx);

    for(auto& ss : state) {
        s.add(ss);
    }

    // GEP processing
    sort domain[] = {ctx.bv_sort(32), ctx.int_sort()};
    func_decl gep = ctx.function("gep", 2, domain, ctx.bv_sort(32));
    addAxiomsForGEP(gep, ctx, s);

    expr assertionPredicate = ctx.bool_const("$isTrue$");
    s.add(implies(assertionPredicate, assertion));

    check_result r = s.check(1, &assertionPredicate);
    return r;
}

bool checkSat(
        const z3::expr& assertion,
        const std::vector<z3::expr>& state,
        z3::context& ctx) {
    return check(assertion, state, ctx) == z3::sat;
}

bool checkUnsat(
        const z3::expr& assertion,
        const std::vector<z3::expr>& state,
        z3::context& ctx) {
    return check(assertion, state, ctx) == z3::unsat;
}

bool checkSatOrUnknown(
        const z3::expr& assertion,
        const std::vector<z3::expr>& state,
        z3::context& ctx) {
    return check(assertion, state, ctx) != z3::unsat;
}

} // namespace borealis
