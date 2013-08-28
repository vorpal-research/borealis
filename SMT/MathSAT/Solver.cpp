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
//        ExecutionContext context(msatef);
//        auto undoed = unlogic::undoThat(e);
//        auto redoed = borealis::SMT<borealis::MathSAT>::doit(undoed, msatef, &context);
//        std::cout << "Original state: " << state << std::endl;
//        std::cout << "Original: " << msat_term_repr(logic::msatimpl::asAxiom(msatstate)) << std::endl;
//        std::cout << "Undoed state: " << undoed << std::endl;
//        std::cout << "Undoed: " << msat_term_repr(logic::msatimpl::asAxiom(redoed)) << std::endl;
//        auto boolE = e.to<Bool>().getUnsafe();
//        auto b = boolE ^ redoed;
//
//        borealis::mathsat::DSolver solver(logic::msatimpl::getEnvironment(b));
//        solver.add(logic::msatimpl::asAxiom(b));
//
//        auto res = solver.check();
//
//        if(res == MSAT_SAT) {
//            borealis::mathsat::ModelIterator iter(solver.env());
//            std::cout << std::endl;
//            while (iter.hasNext()) {
//                iter++;
//                auto& pair = *iter;
//                std::cout << msat_term_repr(pair.first) << " = "
//                          << msat_term_repr(pair.second) << std::endl;
//            }
//        }
//        return res == MSAT_UNSAT;
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

Dynamic Solver::getInterpolant(
        PredicateState::Ptr query,
        PredicateState::Ptr body) {

    using namespace logic;

    TRACE_FUNC;

    ExecutionContext ctx(msatef);
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

        mathsat::Expr interpol = msatimpl::getExpr(msatef.getTrue());

        if (r == MSAT_UNSAT) interpol = s.get_interpolant({B});

        dbgs() << "Got: " << endl
               << interpol << endl;
        return Dynamic(interpol);
    }
}

Dynamic Solver::getSummary(
        const std::vector<Term::Ptr>& args,
        PredicateState::Ptr query,
        PredicateState::Ptr body) {

    using namespace logic;

    TRACE_FUNC;

    ExecutionContext ctx(msatef);
    auto msatbody = SMT<MathSAT>::doit(body, msatef, &ctx);
    auto msatquery = SMT<MathSAT>::doit(query, msatef, &ctx);

    dbgs() << "Summarizing: " << endl
           << msatquery << endl
           << "in: " << endl
           << msatbody << endl;

    mathsat::ISolver s(msatef.unwrap());

    auto B = s.create_and_set_itp_group();
    s.add(msatimpl::asAxiom(msatbody));
    auto Q = s.create_and_set_itp_group();
    s.add(msatimpl::asAxiom(msatquery));

    std::vector<mathsat::Expr> argExprs;
    argExprs.reserve(args.size());
    std::transform(args.begin(), args.end(), std::back_inserter(argExprs),
        [this](const Term::Ptr& arg) -> mathsat::Expr {
            ExecutionContext ctx(msatef);
            return msatimpl::getExpr(
                SMT<MathSAT>::doit(arg, msatef, &ctx)
            );
        }
    );

    {
        TRACE_BLOCK("mathsat::summarize");
        msat_result r = s.check();

        dbgs() << "Acquired result: "
               << ((r == MSAT_SAT) ? "sat" : (r == MSAT_UNSAT) ? "unsat" : "unknown")
               << endl;

        mathsat::Expr interpol = msatimpl::getExpr(msatef.getTrue());

        if (r == MSAT_UNSAT) {
            interpol = s.get_interpolant({B});

        } else if (r == MSAT_SAT) {
            mathsat::DSolver d(msatef.unwrap());
            d.add(msatimpl::asAxiom(   msatbody));
            d.add(msatimpl::asAxiom( ! msatquery));

            auto models = d.diversify(argExprs);
            dbgs() << "Models: " << endl
                   << models << endl;

            s.set_itp_group(Q);
            s.add(models);

            r = s.check();
            if (r == MSAT_UNSAT) interpol = s.get_interpolant({B});

        }

        dbgs() << "Got: " << endl
               << interpol << endl;
        return Dynamic(interpol);
    }
}

} // namespace mathsat_
} // namespace borealis
