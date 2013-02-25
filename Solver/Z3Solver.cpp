/*
 * Z3Solver.cpp
 *
 *  Created on: Oct 31, 2012
 *      Author: ice-phoenix
 */

#include "Solver/Z3Solver.h"
#include "Solver/Logic.hpp"

#include "Logging/logger.hpp"
#include "Logging/tracer.hpp"

namespace borealis {

Z3Solver::Z3Solver(Z3ExprFactory& z3ef) : z3ef(z3ef) {}

z3::check_result Z3Solver::check(
        const PredicateState& q,
        const PredicateState& state) {
    using namespace::z3;
    using logic::Bool;
    using logic::implies;

    TRACE_FUNC;

    solver s(z3ef.unwrap());

    auto z3state = state.toZ3(z3ef);
    s.add(logic::z3impl::asAxiom(z3state));

    dbgs() << "  Query: " << endl << q << endl;
    dbgs() << "  State: " << endl << z3state << endl;

    Bool pred = z3ef.getBoolVar("$CHECK$");
    s.add(logic::z3impl::asAxiom(implies(pred, q.toZ3(z3ef))));

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

bool Z3Solver::checkPathPredicates(
        const PredicateState& state) {
    using namespace::z3;
    using logic::Bool;
    using logic::implies;

    TRACE_FUNC;

    solver s(z3ef.unwrap());

    auto pathPredicates = state.filter([](Predicate::Ptr p) { return p->getType() == PredicateType::PATH; }).toZ3(z3ef);
    auto statePredicates = state.filter([](Predicate::Ptr p) { return p->getType() != PredicateType::PATH; }).toZ3(z3ef);

    s.add(logic::z3impl::getAxiom(statePredicates));

    dbgs() << "  Path predicates: " << endl << pathPredicates << endl;
    dbgs() << "  State predicates: " << endl << statePredicates << endl;

    Bool pred = z3ef.getBoolVar("$CHECK$");
    s.add(logic::z3impl::asAxiom(implies(pred, pathPredicates)));

    {
        TRACE_BLOCK("Calling Z3 check");
        expr pred_e = logic::z3impl::getExpr(pred);
        check_result r = s.check(1, &pred_e);

        dbgs() << "Acquired result: "
               << ((r == z3::sat) ? "sat" : (r == z3::unsat) ? "unsat" : "unknown")
               << endl;

        auto dbg = dbgs();
        dbg << "With:" << endl;
        if (r == z3::sat) {
            dbg << s.get_model() << endl;
        } else if (r == z3::unsat) {
            auto core = s.unsat_core();
            for (size_t i = 0U; i < core.size(); ++i) dbg << core[i] << endl;
        } else {
            dbg << s.reason_unknown() << endl;
        }

        return r != z3::unsat;
    }
}

bool Z3Solver::checkSat(
        const PredicateState& q,
        const PredicateState& state) {
    TRACE_FUNC;
    return check(q, state) == z3::sat;
}

bool Z3Solver::checkUnsat(
        const PredicateState& q,
        const PredicateState& state) {
    TRACE_FUNC;
    return check(q, state) == z3::unsat;
}

bool Z3Solver::checkSatOrUnknown(
        const PredicateState& q,
        const PredicateState& state) {
    TRACE_FUNC;
    return check(q, state) != z3::unsat;
}

} /* namespace borealis */
