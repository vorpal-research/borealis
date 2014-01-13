/*
 * Z3Solver.cpp
 *
 *  Created on: Oct 31, 2012
 *      Author: ice-phoenix
 */

#include "Logging/tracer.hpp"
#include "SMT/Z3/Solver.h"

#include "Util/macros.h"

namespace borealis {
namespace z3_ {

Solver::Solver(ExprFactory& z3ef, unsigned long long memoryStart) :
        z3ef(z3ef), memoryStart(memoryStart) {}

Solver::check_result Solver::check(
        const Bool& z3query_,
        const Bool& z3state_) {

    using namespace logic;

    TRACE_FUNC;

    auto& c = z3ef.unwrap();

    auto params = z3::params(c);
    params.set(":auto_config", true);
    auto smt_tactic = with(z3::tactic(c, "smt"), params);
    auto useful = z3::tactic(c, "reduce-bv-size") & z3::tactic(c, "ctx-simplify");

    auto tactic = useful & smt_tactic;
    auto s = tactic.mk_solver();

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
            auto model = s.get_model();
            dbg << model << endl;
            return std::make_tuple(r, util::just(model), util::nothing(), util::nothing());

        } else if (r == z3::unsat) {
            auto core = s.unsat_core();
            for (size_t i = 0U; i < core.size(); ++i) dbg << core[i] << endl;
            return std::make_tuple(r, util::nothing(), util::just(core), util::nothing());

        } else {
            auto reason = s.reason_unknown();
            dbg << reason << endl;
            return std::make_tuple(r, util::nothing(), util::nothing(), util::just(reason));
        }
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

    z3::check_result res;
    util::option<z3::model> model;
    std::tie(res, model, std::ignore, std::ignore) = check(!z3query, z3state);

    if (res == z3::sat) {
        auto m = model.getUnsafe(); // You shall not fail! (c)

        auto cex = state
        ->filterByTypes({PredicateType::PATH})
        ->filter([this,&m,&ctx](Predicate::Ptr p) -> bool {
            auto z3p = SMT<Z3>::doit(p, z3ef, &ctx);
            auto valid = m.eval(logic::z3impl::asAxiom(z3p));
            auto bValid = util::stringCast<bool>(valid);
            return bValid.getOrElse(false);
        });

        using namespace logging;
        dbgs() << "CEX: "
               << print_predicate_locus_on
               << cex
               << print_predicate_locus_off
               << endl;
    }

    return res != z3::unsat;
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

    z3::check_result res;
    std::tie(res, std::ignore, std::ignore, std::ignore) = check(z3path, z3state);

    return res == z3::unsat;
}

} // namespace z3_
} // namespace borealis

#include "Util/unmacros.h"
