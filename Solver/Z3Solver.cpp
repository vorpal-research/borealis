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
    using namespace::z3;
    using logic::Bool;

    TRACE_FUNC;

    solver s(z3ef.unwrap());

    auto ss = state.toZ3(z3ef);
    s.add(ss.first.toAxiom());
    s.add(ss.second.toAxiom());

    dbgs() << "  Path predicates: " << endl << ss.first << endl;
    dbgs() << "  State predicates: " << endl << ss.second << endl;

    expr pred = z3ef.getBoolVar("$CHECK$").toAxiom();
    s.add(implies(pred, q.toZ3(z3ef).toAxiom()));

    {
        TRACE_BLOCK("Calling Z3 check");
        check_result r = s.check(1, &pred);
        dbgs() << "Acquired result: "
               << ((r == z3::sat)? "sat" : (r == z3::unsat)? "unsat" : "unknown")
               << endl;
        return r;
    }
}

bool Z3Solver::checkPathPredicates(
        const PredicateState& state) {
    using namespace::z3;

    TRACE_FUNC;

    solver s(z3ef.unwrap());

    auto ss = state.toZ3(z3ef);
    s.add(ss.second.toAxiom());

    dbgs() << "  Path predicates: " << endl << ss.first << endl;
    dbgs() << "  State predicates: " << endl << ss.second << endl;

    expr pred = z3ef.getBoolVar("$CHECK$").toAxiom();
    s.add(implies(pred, ss.first.toAxiom()));

    {
        TRACE_BLOCK("Calling Z3 check");
        check_result r = s.check(1, &pred);

        dbgs() << "Acquired result: "
               << ((r == z3::sat) ? "sat" : (r == z3::unsat) ? "unsat" : "unknown")
               << endl;

        auto dbg = dbgs();
        dbg << "With:" << endl;
        if (r == z3::sat) dbg << s.get_model() << endl;
        else {
            auto core = s.unsat_core();
            for (size_t i = 0U; i < core.size(); ++i ) dbg << core[i] << endl;
        }

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

} /* namespace borealis */
