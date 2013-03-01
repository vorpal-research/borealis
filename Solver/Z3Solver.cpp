/*
 * Z3Solver.cpp
 *
 *  Created on: Oct 31, 2012
 *      Author: ice-phoenix
 */

#include "Logging/logger.hpp"
#include "Logging/tracer.hpp"
#include "Solver/Logic.hpp"
#include "Solver/Z3Solver.h"

namespace borealis {

Z3Solver::Z3Solver(Z3ExprFactory& z3ef) : z3ef(z3ef) {}

z3::check_result Z3Solver::check(
        const logic::Bool& z3query,
        const logic::Bool& z3state) {
    using namespace::z3;
    using logic::Bool;
    using logic::implies;

    TRACE_FUNC;

    solver s(z3ef.unwrap());

    s.add(logic::z3impl::asAxiom(z3state));

    dbgs() << "  Query: " << endl << z3query << endl;
    dbgs() << "  State: " << endl << z3state << endl;

    Bool pred = z3ef.getBoolVar("$CHECK$");
    s.add(logic::z3impl::asAxiom(implies(pred, z3query)));

    {
        TRACE_BLOCK("Calling Z3 check");
        auto dbg = dbgs();

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
        } else {
            dbg << s.reason_unknown() << endl;
        }

        return r;
    }
}

bool Z3Solver::checkViolated(
        const PredicateState& query,
        const PredicateState& state) {
    TRACE_FUNC;

    auto z3query = !query.toZ3(z3ef);
    auto z3state = state.toZ3(z3ef);
    return check(z3query, z3state) != z3::unsat;
}

bool Z3Solver::checkPathPredicates(
        const PredicateState& path,
        const PredicateState& state) {
    TRACE_FUNC;

    auto z3query = path.toZ3(z3ef);
    auto z3state = state.toZ3(z3ef);
    return check(z3query, z3state) == z3::unsat;
}

} /* namespace borealis */
