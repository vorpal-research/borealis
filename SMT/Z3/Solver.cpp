/*
 * Z3Solver.cpp
 *
 *  Created on: Oct 31, 2012
 *      Author: ice-phoenix
 */

#include "Logging/tracer.hpp"
#include "SMT/Z3/Solver.h"

namespace borealis {
namespace z3_ {

Solver::Solver(ExprFactory& z3ef, unsigned long long memoryStart) :
        z3ef(z3ef), memoryStart(memoryStart) {}

z3::check_result Solver::check(
        const Bool& z3query_,
        const Bool& z3state_) {

    using namespace logic;

    TRACE_FUNC;

    z3::solver s(z3ef.unwrap());
    auto dbg = dbgs();

    auto z3state = z3state_.simplify();
    auto z3query = z3query_.simplify();

    s.add(z3impl::asAxiom(z3state));

    dbg << "  Query: " << endl << z3query << endl;
    dbg << "  State: " << endl << z3state << endl;
    dbg << end;

    Bool pred = z3ef.getBoolVar("$CHECK$");
    s.add(z3impl::asAxiom(implies(pred, z3query)));

    {
        TRACE_BLOCK("z3::check");

        z3::expr pred_e = logic::z3impl::getExpr(pred);
        z3::check_result r = s.check(1, &pred_e);
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

bool Solver::isViolated(
        PredicateState::Ptr query,
        PredicateState::Ptr state) {
    TRACE_FUNC;

    dbgs() << "Checking query: " << endl
           << query << endl
           << "in: " << endl
           << state << endl;

    ExecutionContext ctx(z3ef, memoryStart);
    auto z3state = SMT<Z3>::doit(state, z3ef, &ctx);
    auto z3query = SMT<Z3>::doit(query, z3ef, &ctx);
    return check(!z3query, z3state) != z3::unsat;
}

bool Solver::isPathImpossible(
        PredicateState::Ptr path,
        PredicateState::Ptr state) {
    TRACE_FUNC;

    dbgs() << "Checking path: " << endl
           << path << endl
           << "in: " << endl
           << state << endl;

    ExecutionContext ctx(z3ef, memoryStart);
    auto z3state = SMT<Z3>::doit(state, z3ef, &ctx);
    auto z3path = SMT<Z3>::doit(path, z3ef, &ctx);
    return check(z3path, z3state) == z3::unsat;
}

} // namespace z3_
} // namespace borealis
