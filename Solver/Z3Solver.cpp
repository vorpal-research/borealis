/*
 * Z3Solver.cpp
 *
 *  Created on: Oct 31, 2012
 *      Author: ice-phoenix
 */

#include "Logging/tracer.hpp"
#include "Solver/Logic.hpp"
#include "Solver/Z3Solver.h"

namespace borealis {

Z3Solver::Z3Solver(Z3ExprFactory& z3ef) : z3ef(z3ef) {}

z3::check_result Z3Solver::check(
        const logic::Bool& z3query_,
        const logic::Bool& z3state_) {
    using namespace::z3;
    using logic::Bool;
    using logic::implies;

    TRACE_FUNC;

    solver s(z3ef.unwrap());
    auto dbg = dbgs();

    auto z3state = z3state_.simplify();
    auto z3query = z3query_.simplify();

    s.add(logic::z3impl::asAxiom(z3state));

    dbg << "  Query: " << endl << z3query << endl;
    dbg << "  State: " << endl << z3state << endl;
    dbg << end;

    Bool pred = z3ef.getBoolVar("$CHECK$");
    s.add(logic::z3impl::asAxiom(implies(pred, z3query)));

    {
        TRACE_BLOCK("z3::check");

        expr pred_e = logic::z3impl::getExpr(pred);
        check_result r = s.check(1, &pred_e);
        dbg << "Acquired result: "
            << ((r == z3::sat) ? "sat" : (r == z3::unsat) ? "unsat" : "unknown")
            << endl;

        dbg << "With:" << endl;
        if (r == z3::sat) {
            dbg << s.get_model() << endl;
        } else if (r == z3::unsat) {
            auto core = s.unsat_core();
            for (size_t i = 0U; i < core.size(); ++i) dbg << core[i] << endl;

            // dbg << "PROOF!: " << s.proof() << endl;

        } else {
            dbg << s.reason_unknown() << endl;
        }

        return r;
    }
}

bool Z3Solver::isViolated(
        PredicateState::Ptr query,
        PredicateState::Ptr state) {
    TRACE_FUNC;

    dbgs() << "Checking query: " << endl
           << query << endl
           << "in: " << endl
           << state << endl;

    ExecutionContext ctx(z3ef);
    auto z3state = state->toZ3(z3ef, &ctx);
    auto z3query = !query->toZ3(z3ef, &ctx);
    return check(z3query, z3state) != z3::unsat;
}

bool Z3Solver::isPathImpossible(
        PredicateState::Ptr path,
        PredicateState::Ptr state) {
    TRACE_FUNC;

    dbgs() << "Checking path: " << endl
           << path << endl
           << "in: " << endl
           << state << endl;

    ExecutionContext ctx(z3ef);
    auto z3state = state->toZ3(z3ef, &ctx);
    auto z3path = path->toZ3(z3ef, &ctx);
    return check(z3path, z3state) == z3::unsat;
}

} /* namespace borealis */
