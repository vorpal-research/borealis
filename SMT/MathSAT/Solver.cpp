/*
 * Solver.cpp
 *
 *  Created on: Aug 5, 2013
 *      Author: sam
 */


#include "Logging/tracer.hpp"
#include "SMT/MathSAT/Solver.h"

namespace borealis {
namespace mathsat_ {

Solver::Solver(ExprFactory& msatef) : msatef(msatef) {}

msat_result Solver::check(
        const Bool& msatquery_,
        const Bool& msatstate_) {

    using namespace logic;

    TRACE_FUNC;

    mathsat::Solver s(msatef.unwrap());
    s.create_and_set_itp_group();
    auto dbg = dbgs();

    s.add(msatimpl::asAxiom(msatstate_));

    dbg << "  Query: " << endl << msatquery_ << endl;
    dbg << "  State: " << endl << msatstate_ << endl;
    dbg << end;
    Bool pred = msatef.getBoolVar("$CHECK$");
    s.add(msatimpl::asAxiom(implies(pred, msatquery_)));
//	s.add(msatimpl::asAxiom(msatquery_));

    {
        TRACE_BLOCK("mathsat::check");

        mathsat::Expr pred_e = logic::msatimpl::getExpr(pred);
        msat_result r = s.check({pred_e});
//        msat_result r = s.check();
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

mathsat::Expr Solver::getInterpolant(
		PredicateState::Ptr query,
        PredicateState::Ptr state) {

	using namespace logic;

	TRACE_FUNC;

	ExecutionContext ctx(msatef);
	auto msatstate = SMT<MathSAT>::doit(state, msatef, &ctx);
	auto msatquery = SMT<MathSAT>::doit(query, msatef, &ctx);

	dbgs() 	<< "Interpolating:" << endl
			<< msatquery << endl
			<< "in:" << endl
			<< msatstate << endl;

	mathsat::Solver s(msatef.unwrap());

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
			return msatef.unwrap().bool_val(false);
		}

		auto interpol = s.get_interpolant({a});
		return interpol;
	}
}

} // namespace mathsat_
} // namespace borealis

