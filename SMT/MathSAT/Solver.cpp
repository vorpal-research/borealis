/*
 * Solver.cpp
 *
 *  Created on: Aug 5, 2013
 *      Author: sam
 */

#include "Logging/tracer.hpp"
#include "SMT/MathSAT/Solver.h"
#include "SMT/MathSAT/Unlogic/Unlogic.h"
#include "SMT/Z3/Solver.h"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {
namespace mathsat_ {

USING_SMT_IMPL(MathSAT);

Solver::Solver(ExprFactory& msatef, unsigned long long memoryStart, unsigned long long memoryEnd) :
        msatef(msatef), memoryStart(memoryStart), memoryEnd(memoryEnd) {}

Solver::check_result Solver::check(
        const Bool& msatquery_,
        const Bool& msatstate_) {

    using namespace logic;

    TRACE_FUNC;

    mathsat::Solver s(msatef.unwrap());
    auto dbg = dbgs();

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

        dbg << "With:" << endl;
        if (r == MSAT_SAT) {
            auto model = s.get_model();
            dbg << util::viewContainer(model).toVector() << endl;
            return std::make_tuple(r, util::just(model), util::nothing(), util::nothing());

        } else if (r == MSAT_UNSAT) {
            auto core = s.unsat_core();
            for (size_t i = 0U; i < core.size(); ++i) dbg << core[i] << endl;
            return std::make_tuple(r, util::nothing(), util::just(core), util::nothing());

        } else {
            std::string reason{"UNKNOWN"}; // XXX: Extraction for reason???
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

    ExecutionContext ctx(msatef, memoryStart, memoryEnd);
    auto msatstate = SMT<MathSAT>::doit(state, msatef, &ctx);
    auto msatquery = SMT<MathSAT>::doit(query, msatef, &ctx);

    msat_result res;
    util::option<mathsat::Model> model;
    std::tie(res, model, std::ignore, std::ignore) = check(!msatquery, msatstate);

    if (res == MSAT_SAT) {
        auto m = model.getUnsafe(); // You shall not fail! (c)

        auto cex = state
        ->filterByTypes({PredicateType::PATH})
        ->filter([this,&m,&ctx](Predicate::Ptr p) -> bool {
            auto msatp = SMT<MathSAT>::doit(p, msatef, &ctx);
            auto valid = m.eval(logic::msatimpl::asAxiom(msatp));
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

    return res != MSAT_UNSAT;
}

bool Solver::isPathImpossible(
        PredicateState::Ptr path,
        PredicateState::Ptr state) {
    TRACE_FUNC;

    dbgs() << "Checking path: " << endl
           << path << endl
           << "in: " << endl
           << state << endl;

    ExecutionContext ctx(msatef, memoryStart, memoryEnd);
    auto msatstate = SMT<MathSAT>::doit(state, msatef, &ctx);
    auto msatpath = SMT<MathSAT>::doit(path, msatef, &ctx);

    msat_result res;
    std::tie(res, std::ignore, std::ignore, std::ignore) = check(msatpath, msatstate);

    return res == MSAT_UNSAT;
}

Dynamic Solver::getInterpolant(
        PredicateState::Ptr query,
        PredicateState::Ptr body) {

    using namespace logic;

    TRACE_FUNC;

    ExecutionContext ctx(msatef, memoryStart, memoryEnd);
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

////////////////////////////////////////////////////////////////////////////////
// Summarizing
////////////////////////////////////////////////////////////////////////////////

unsigned getCountLimit() {
    static config::ConfigEntry<int> CountLimit("summary", "sampling-count-limit");
    return CountLimit.get(16);
}

unsigned getAttemptLimit() {
    static config::ConfigEntry<int> AttemptLimit("summary", "sampling-attempt-limit");
    return AttemptLimit.get(100);
}

std::vector<Term::Ptr> getPathVars(PredicateState::Ptr state) {
    USING_SMT_LOGIC(MathSAT);

    PredicateState::Ptr pathState;
    std::tie(pathState, std::ignore) = state->splitByTypes({PredicateType::PATH});

    std::vector<Term::Ptr> pathTerms;
    pathState->map([&pathTerms](Predicate::Ptr p) -> Predicate::Ptr {
        if (auto pp = llvm::dyn_cast<EqualityPredicate>(p)) {
            pathTerms.push_back(pp->getLhv());
            return p;
        } else if (auto pp = llvm::dyn_cast<InequalityPredicate>(p)) {
            pathTerms.push_back(pp->getLhv());
            return p;
        } else if (auto pp = llvm::dyn_cast<DefaultSwitchCasePredicate>(p)) {
            return p;
        } else {
            BYE_BYE(Predicate::Ptr, "Wrong path predicate type. " + p->toString());
        }
    });

    return pathTerms;
}

Bool getProbeModels(
        ExprFactory& msatef,
        Bool body,
        Bool query,
        const std::vector<mathsat::Expr>& args,
        const std::vector<mathsat::Expr>& pathVars) {
    using namespace logic;

    static auto countLimit = getCountLimit();
    static auto attemptLimit = getAttemptLimit();

    mathsat::DSolver d(msatef.unwrap());
    d.add(msatimpl::asAxiom( body  ));
    d.add(msatimpl::asAxiom( query ));

    auto ms = msatef.getFalse();
    auto count = 0U;
    auto attempt = 0U;

    auto fullCount = 0U; // for logging

    std::vector<mathsat::Expr>  dvrs(args);
    dvrs.insert(dvrs.end(), pathVars.begin(), pathVars.end());

    while (count < countLimit && attempt < attemptLimit) {
        auto models = d.diversify_unsafe(dvrs, args, countLimit * 2);

        for (const auto& m : models) {
            ++fullCount;    // for logging

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

    dbgs() << "Attempts: " << attempt << endl
           << "Count: " << fullCount << endl;

    return ms;
}


Bool Solver::probeZ3(PredicateState::Ptr body,
             PredicateState::Ptr query,
             const std::vector<Term::Ptr>& args,
             const std::vector<Term::Ptr>& pathVars) {
    std::vector<Term::Ptr> dvrs(args);
    dvrs.insert(dvrs.end(), pathVars.begin(), pathVars.end());

    z3_::ExprFactory z3ef;
    z3_::Solver z3solver(z3ef, memoryStart, memoryEnd);
    auto modelState = z3solver.probeModels(body, query, dvrs, args);

    ExecutionContext ctx(msatef, memoryStart, memoryEnd);
    return SMT<MathSAT>::doit(modelState, msatef, &ctx);
}


Dynamic Solver::getSummary(
        const std::vector<Term::Ptr>& args,
        PredicateState::Ptr query,
        PredicateState::Ptr body) {

    using namespace logic;

    TRACE_FUNC;

    ExecutionContext ctx(msatef, memoryStart, memoryEnd);
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


    auto toExpr =
        [this](const Term::Ptr& term) -> mathsat::Expr {
            ExecutionContext ctx(msatef, memoryStart, memoryEnd);
            return msatimpl::getExpr(
                    SMT<MathSAT>::doit(term, msatef, &ctx)
            );
        };

    auto argExprs = util::viewContainer(args).map(toExpr).toVector();
    auto pathVars = getPathVars(body);
    auto pathExprs = util::viewContainer(pathVars).map(toExpr).toVector();

    {
        TRACE_BLOCK("mathsat::summarize");
        msat_result r = s.check();

        dbgs() << "Acquired result: "
               << ((r == MSAT_SAT) ? "sat" : (r == MSAT_UNSAT) ? "unsat" : "unknown")
               << endl;

        mathsat::Expr interpol = msatimpl::asAxiom(msatef.getTrue());

        static config::ConfigEntry<bool> ModelSampling("summary", "model-sampling");
        static config::ConfigEntry<std::string> SamplingSolver("summary", "sampling-solver");

        if (r == MSAT_UNSAT) {
            interpol = s.get_interpolant({B});

        } else if (r == MSAT_SAT && ModelSampling.get(true)) {
            std::string dbgStr = "Model:\n";
            for (auto me: s.get_model()) {
                dbgStr += me.term.decl().name() + std::string(" = ")
                       + me.value.decl().name() + std::string("\n");
            }
            dbgs() << dbgStr;

            auto ms = msatef.getTrue();
            if (SamplingSolver.get("mathsat") == "mathsat") {
                ms = getProbeModels(msatef, msatbody, msatquery, argExprs, pathExprs);
            } else { // Z3
                ms = probeZ3(body, query, args, pathVars);
            }

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

    ExecutionContext ctx(msatef, memoryStart, memoryEnd);
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

    auto toExpr =
        [this](const Term::Ptr& term) -> mathsat::Expr {
            ExecutionContext ctx(msatef, memoryStart, memoryEnd);
            return msatimpl::getExpr(
                    SMT<MathSAT>::doit(term, msatef, &ctx)
            );
        };

    auto argExprs = util::viewContainer(args).map(toExpr).toVector();
    auto pathVars = getPathVars(body);
    auto pathExprs = util::viewContainer(pathVars).map(toExpr).toVector();

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
            auto ms = getProbeModels(msatef, msatbody, msatquery, argExprs, pathExprs);

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
