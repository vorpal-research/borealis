/*
 * Z3Solver.cpp
 *
 *  Created on: Oct 31, 2012
 *      Author: ice-phoenix
 */

#include "Z3Solver.h"

namespace borealis {

Z3Solver::Z3Solver(Z3ExprFactory& z3ef) : z3ef(z3ef) {}

z3::check_result Z3Solver::check(
        const Query& q,
        const PredicateState& state) {
    using namespace::z3;

    solver s(z3ef.unwrap());
    addGEPAxioms(s);

    auto ss = state.toZ3(z3ef);
    s.add(ss.first);
    s.add(ss.second);

    expr pred = z3ef.getBoolVar("$CHECK$");
    s.add(implies(pred, q.toZ3(z3ef)));

    check_result r = s.check(1, &pred);
    return r;
}

bool Z3Solver::checkPathPredicates(
        const PredicateState& state) {
    using namespace::z3;

    solver s(z3ef.unwrap());
    addGEPAxioms(s);

    auto ss = state.toZ3(z3ef);
    s.add(ss.second);

    expr pred = z3ef.getBoolVar("$CHECK$");
    s.add(implies(pred, ss.first));

    check_result r = s.check(1, &pred);
    return r == z3::unsat;
}

bool Z3Solver::checkSat(
        const Query& q,
        const PredicateState& state) {
    return check(q, state) == z3::sat;
}

bool Z3Solver::checkUnsat(
        const Query& q,
        const PredicateState& state) {
    return check(q, state) == z3::unsat;
}

bool Z3Solver::checkSatOrUnknown(
        const Query& q,
        const PredicateState& state) {
    return check(q, state) != z3::unsat;
}

void Z3Solver::addGEPAxioms(z3::solver& s) {
    using namespace::z3;

    auto& ctx = z3ef.unwrap();

    auto gep = z3ef.getGEPFunction();
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

} /* namespace borealis */
