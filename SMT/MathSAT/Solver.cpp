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

USING_SMT_IMPL(MathSAT);

Solver::Solver(ExprFactory& msatef, unsigned long long memoryStart) :
        msatef(msatef), memoryStart(memoryStart) {}

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

    ExecutionContext ctx(msatef, memoryStart);
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

    ExecutionContext ctx(msatef, memoryStart);
    auto msatstate = SMT<MathSAT>::doit(state, msatef, &ctx);
    auto msatpath = SMT<MathSAT>::doit(path, msatef, &ctx);
    return check(msatpath, msatstate) == MSAT_UNSAT;
}

Dynamic Solver::getInterpolant(
        PredicateState::Ptr query,
        PredicateState::Ptr body) {

    using namespace logic;

    TRACE_FUNC;

    ExecutionContext ctx(msatef, memoryStart);
    auto msatbody = SMT<MathSAT>::doit(body, msatef, &ctx);
    auto msatquery = SMT<MathSAT>::doit(query, msatef, &ctx);

    dbgs() << "Interpolating: " << endl
           << msatquery << endl
           << "in: " << endl
           << msatbody << endl;

    mathsat::ISolver s(msatef.unwrap());

    auto B = s.create_and_set_itp_group();
    s.add(msatimpl::asAxiom(msatbody));
    s.create_and_set_itp_group();
    s.add(msatimpl::asAxiom(msatquery));

    {
        TRACE_BLOCK("mathsat::interpol");
        msat_result r = s.check();

        dbgs() << "Acquired result: "
               << ((r == MSAT_SAT) ? "sat" : (r == MSAT_UNSAT) ? "unsat" : "unknown")
               << endl;

        mathsat::Expr interpol = msatimpl::asAxiom(msatef.getTrue());

        if (r == MSAT_UNSAT) interpol = s.get_interpolant({B});

        dbgs() << "Got: " << endl
               << interpol << endl;
        return Dynamic(interpol);
    }
}

Bool getProbeModels(
        ExprFactory& msatef,
        Bool body,
        Bool query,
        const std::vector<mathsat::Expr>& args,
        unsigned int countLimit = 16,
        unsigned int attemptLimit = 100) {
    using namespace logic;

    mathsat::DSolver d(msatef.unwrap());
    d.add(msatimpl::asAxiom( body  ));
    d.add(msatimpl::asAxiom( query ));

    auto ms = msatef.getFalse();
    auto count = 0U;
    auto attempt = 0U;

    while (count < countLimit && attempt < attemptLimit) {
        auto models = d.diversify_unsafe(args, countLimit * 2);

        for (const auto& m : models) {
            mathsat::ISolver s(msatef.unwrap());
            s.create_and_set_itp_group();
            s.add(msatimpl::asAxiom(   body  ));
            s.add(msatimpl::asAxiom( ! query ));
            s.add(msatimpl::asAxiom(   m ));

            if (MSAT_SAT == s.check()) continue;

            ms = ms || m;
            ++count;

            if (count >= countLimit) break;
        }

        ++attempt;
    }

    return ms;
}

Dynamic Solver::getSummary(
        const std::vector<Term::Ptr>& args,
        PredicateState::Ptr query,
        PredicateState::Ptr body) {

    using namespace logic;

    TRACE_FUNC;

    ExecutionContext ctx(msatef, memoryStart);
    auto msatbody = SMT<MathSAT>::doit(body, msatef, &ctx);
    auto msatquery = SMT<MathSAT>::doit(query, msatef, &ctx);

    dbgs() << "Summarizing: " << endl
           << msatquery << endl
           << "in: " << endl
           << msatbody << endl;

    mathsat::ISolver s(msatef.unwrap());

    auto B = s.create_and_set_itp_group();
    s.add(msatimpl::asAxiom(   msatbody));
    auto Q = s.create_and_set_itp_group();
    s.add(msatimpl::asAxiom( ! msatquery));

    auto argExprs = util::viewContainer(args)
        .map([this](const Term::Ptr& arg) -> mathsat::Expr {
            ExecutionContext ctx(msatef, memoryStart);
            return msatimpl::getExpr(
                    SMT<MathSAT>::doit(arg, msatef, &ctx)
            );
        })
        .toVector();

    {
        TRACE_BLOCK("mathsat::summarize");
        msat_result r = s.check();

        dbgs() << "Acquired result: "
               << ((r == MSAT_SAT) ? "sat" : (r == MSAT_UNSAT) ? "unsat" : "unknown")
               << endl;

        mathsat::Expr interpol = msatimpl::asAxiom(msatef.getTrue());

        if (r == MSAT_UNSAT) {
            interpol = s.get_interpolant({B});

        } else if (r == MSAT_SAT) {
            auto ms = getProbeModels(msatef, msatbody, msatquery, argExprs);

            dbgs() << "Probes: " << endl
                   << ms << endl;

            s.set_itp_group(Q);
            s.add(msatimpl::asAxiom(ms));

            r = s.check();
            if (r == MSAT_UNSAT) interpol = s.get_interpolant({B});
            else dbgs() << "Oops, got MSAT_SAT for (B && not Q && models)..." << endl;

        }

        dbgs() << "Got: " << endl
               << interpol << endl;
        return Dynamic(interpol);
    }
}

Dynamic Solver::getContract(
        const std::vector<Term::Ptr>& args,
        PredicateState::Ptr query,
        PredicateState::Ptr body) {

    using namespace logic;

    TRACE_FUNC;

    ExecutionContext ctx(msatef, memoryStart);
    auto msatbody = SMT<MathSAT>::doit(body, msatef, &ctx);
    auto msatquery = SMT<MathSAT>::doit(query, msatef, &ctx);

    dbgs() << "Generating contract for: " << endl
           << msatquery << endl
           << "in: " << endl
           << msatbody << endl;

    mathsat::ISolver s(msatef.unwrap());

    auto B = s.create_and_set_itp_group();

    /* auto Q = */ s.create_and_set_itp_group();
    s.add(msatimpl::asAxiom(   msatbody));
    s.add(msatimpl::asAxiom( ! msatquery));

    auto argExprs = util::viewContainer(args)
        .map([this](const Term::Ptr& arg) -> mathsat::Expr {
            ExecutionContext ctx(msatef, memoryStart);
            return msatimpl::getExpr(
                    SMT<MathSAT>::doit(arg, msatef, &ctx)
            );
        })
        .toVector();

    {
        TRACE_BLOCK("mathsat::contract");
        msat_result r = s.check();

        dbgs() << "Acquired result: "
               << ((r == MSAT_SAT) ? "sat" : (r == MSAT_UNSAT) ? "unsat" : "unknown")
               << endl;

        mathsat::Expr interpol = msatimpl::asAxiom(msatef.getTrue());

        if (r == MSAT_UNSAT) {
            BYE_BYE(Dynamic, "No contract exists for UNSAT formula");

        } else if (r == MSAT_SAT) {
            auto ms = getProbeModels(msatef, msatbody, msatquery, argExprs);

            dbgs() << "Probes: " << endl
                   << ms << endl;

            s.set_itp_group(B);
            s.add(msatimpl::asAxiom(ms));

            r = s.check();
            if (r == MSAT_UNSAT) interpol = s.get_interpolant({B});
            else dbgs() << "Oops, got MSAT_SAT for (B && not Q && models)..." << endl;

        }

        dbgs() << "Got: " << endl
               << interpol << endl;
        return Dynamic(interpol);
    }
}

} // namespace mathsat_
} // namespace borealis

#include "Util/unmacros.h"
