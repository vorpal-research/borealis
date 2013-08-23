/*
 * Solver.cpp
 *
 *  Created on: Aug 5, 2013
 *      Author: sam
 */

#include "Logging/tracer.hpp"
#include "SMT/MathSAT/Solver.h"
#include "SMT/MathSAT/Unlogic/Unlogic.h"
#include "Util/util.h"
#include "Util/macros.h"

namespace borealis {
namespace mathsat_ {

Solver::Solver(ExprFactory& msatef) : msatef(msatef) {}

msat_result Solver::check(
        const Bool& msatquery_,
        const Bool& msatstate_) {

    using namespace logic;

    TRACE_FUNC;

    auto dbg = dbgs();

    mathsat::Solver s(msatef.unwrap());

    s.add(msatimpl::asAxiom(msatstate_));

    dbg << "  Query: " << endl << msatquery_ << endl;
    dbg << "  State: " << endl << msatstate_ << endl;
    dbg << end;

    Bool pred = msatef.getBoolVar("$CHECK$");
    s.add(msatimpl::asAxiom(implies(pred, msatquery_)));

    {
        TRACE_BLOCK("mathsat::check");

        mathsat::Expr pred_e = logic::msatimpl::getExpr(pred);
        msat_result r = s.check({pred_e});
        dbg << "Acquired result: "
            << ((r == MSAT_SAT) ? "sat" : (r == MSAT_UNSAT) ? "unsat" : "unknown")
            << endl;

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

    ExecutionContext ctx(msatef);
    auto msatstate = SMT<MathSAT>::doit(state, msatef, &ctx);
    auto msatquery = SMT<MathSAT>::doit(query, msatef, &ctx);

//    auto check_undo = [&](Dynamic e)->bool {
//        auto undoed = unlogic::undoThat(e);
//        auto redoed = borealis::SMT<borealis::MathSAT>::doit(undoed, msatef, &ctx);
//        std::cout << "Original state: " << state << std::endl;
//        std::cout << "Original: " << msat_term_repr(logic::msatimpl::getExpr(msatstate)) << std::endl;
//        std::cout << "Undoed state: " << undoed << std::endl;
//        std::cout << "Undoed: " << msat_term_repr(logic::msatimpl::getExpr(redoed)) << std::endl;
//        auto b = (e == Dynamic(redoed));
//
//        borealis::mathsat::Solver solver(logic::msatimpl::getEnvironment(b));
//        solver.add(logic::msatimpl::getAxiom(b));
//        solver.add(!logic::msatimpl::getExpr(b));
//        return solver.check() == MSAT_UNSAT;
//    };
//    ASSERTC(check_undo(msatstate));


    return check(!msatquery, msatstate) != MSAT_UNSAT;
}

bool Solver::isPathImpossible(
        PredicateState::Ptr path,
        PredicateState::Ptr state) {
    TRACE_FUNC;

    dbgs() << "Checking path: " << endl
           << path << endl
           << "in: " << endl
           << state << endl;

    ExecutionContext ctx(msatef);
    auto msatstate = SMT<MathSAT>::doit(state, msatef, &ctx);
    auto msatpath = SMT<MathSAT>::doit(path, msatef, &ctx);
    return check(msatpath, msatstate) == MSAT_UNSAT;
}

USING_SMT_LOGIC(MathSAT);

Dynamic Solver::getInterpolant(
        PredicateState::Ptr query,
        PredicateState::Ptr state) {

    using namespace logic;

    TRACE_FUNC;

    ExecutionContext ctx(msatef);
    auto msatstate = SMT<MathSAT>::doit(state, msatef, &ctx);
    auto msatquery = SMT<MathSAT>::doit(query, msatef, &ctx);

    dbgs()     << "Interpolating:" << endl
            << msatquery << endl
            << "in:" << endl
            << msatstate << endl;

    mathsat::ISolver s(msatef.unwrap());

    auto a = s.create_and_set_itp_group();
    s.add(msatimpl::asAxiom(msatstate));

    s.create_and_set_itp_group();
    s.add(msatimpl::asAxiom(msatquery));

    {
        TRACE_BLOCK("mathsat::interpol");
        msat_result r = s.check();

        dbgs() << "Acquired result: "
            << ((r == MSAT_SAT) ? "sat" : (r == MSAT_UNSAT) ? "unsat" : "unknown")
            << endl;

        if (r != MSAT_UNSAT) {
            return msatef.getTrue();
        }

        auto interpol = s.get_interpolant({a});
        return Dynamic(interpol);
    }
}

} // namespace mathsat_
} // namespace borealis
