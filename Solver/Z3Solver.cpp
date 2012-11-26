/*
 * Z3Solver.cpp
 *
 *  Created on: Oct 31, 2012
 *      Author: ice-phoenix
 */

#include "Z3Solver.h"

#include "Logging/logger.hpp"
#include "Logging/tracer.hpp"

namespace borealis {

Z3Solver::Z3Solver(Z3ExprFactory& z3ef) : z3ef(z3ef) {}

z3::check_result Z3Solver::check(
        const Query& q,
        const PredicateState& state) {

    TRACE_FUNC;

    using namespace::z3;

    solver s(z3ef.unwrap());
    addGEPAxioms(s);

    auto ss = state.toZ3(z3ef);
    s.add(ss.first.simplify());
    s.add(ss.second.simplify());

    dbgs() << "  Path predicates: " << endl << ss.first.simplify() << endl;
    dbgs() << "  State predicates: " << endl << ss.second.simplify() << endl;

    expr pred = z3ef.getBoolVar("$CHECK$");
    s.add(implies(pred, q.toZ3(z3ef)));

    {
        TRACE_BLOCK("Calling Z3 check");
        check_result r = s.check(1, &pred);
        return r;
    }
}

bool Z3Solver::checkPathPredicates(
        const PredicateState& state) {
    using namespace::z3;

    TRACE_FUNC;

    solver s(z3ef.unwrap());
    addGEPAxioms(s);

    auto ss = state.toZ3(z3ef);
    s.add(ss.second.simplify());

    dbgs() << "  Path predicates: " << endl << ss.first.simplify() << endl;
    dbgs() << "  State predicates: " << endl << ss.second.simplify() << endl;

    expr pred = z3ef.getBoolVar("$CHECK$");
    s.add(implies(pred, ss.first));

    {
        TRACE_BLOCK("Calling Z3 check");
        check_result r = s.check(1, &pred);
        return r != z3::unsat;
    }
}

bool Z3Solver::checkSat(
        const Query& q,
        const PredicateState& state) {
    TRACE_FUNC;
    return check(q, state) == z3::sat;
}

bool Z3Solver::checkUnsat(
        const Query& q,
        const PredicateState& state) {
    TRACE_FUNC;
    return check(q, state) == z3::unsat;
}

bool Z3Solver::checkSatOrUnknown(
        const Query& q,
        const PredicateState& state) {
    TRACE_FUNC;
    return check(q, state) != z3::unsat;
}

void Z3Solver::addGEPAxioms(z3::solver& s) {
    using namespace::z3;

    TRACE_FUNC;

    auto& ctx = z3ef.unwrap();

    auto gep = z3ef.getGEPFunction();
    auto ptr_sort = z3ef.getPtrSort();

    auto ptr = to_expr(ctx, Z3_mk_bound(ctx, 0, ptr_sort));
    auto freevar = to_expr(ctx, Z3_mk_bound(ctx, 1, ctx.int_sort()));
    auto null = z3ef.getNullPtr();

    auto body = implies(
            ptr != null,
            gep(ptr, freevar) != null);

    Z3_sort sort_array[] = {Z3_sort(ctx.int_sort()), Z3_sort(ptr_sort)};
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
