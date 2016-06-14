/*
 * Solver.cpp
 *
 *  Created on: Aug 5, 2013
 *      Author: sam
 */

#include "State/Transformer/VariableCollector.h"
#include "Logging/tracer.hpp"
#include "SMT/MathSAT/Solver.h"
#include "SMT/MathSAT/Unlogic/Unlogic.h"
#include "SMT/Z3/Solver.h"
#include "Util/util.h"

#include "Factory/Nest.h"

#include "Util/macros.h"

namespace borealis {
namespace mathsat_ {

using namespace borealis::smt;

USING_SMT_IMPL(MathSAT);

static config::BoolConfigEntry gather_smt_models("analysis", "collect-models");
static config::BoolConfigEntry gather_msat_models("analysis", "collect-mathsat-models");

static config::BoolConfigEntry model_sampling("summary", "model-sampling");
static config::StringConfigEntry sampling_solver("summary", "sampling-solver");

Solver::Solver(ExprFactory& msatef, unsigned long long memoryStart, unsigned long long memoryEnd) :
        msatef(msatef), memoryStart(memoryStart), memoryEnd(memoryEnd) {}

Solver::check_result Solver::check(
        const Bool& msatquery_,
        const Bool& msatstate_) {

    using namespace logic;

    TRACE_FUNC;

    mathsat::Solver s{ msatef.unwrap() };
    auto&& dbg = dbgs();

    s.add(msatstate_.asAxiom());

    dbg << "  Query: " << endl << msatquery_ << endl;
    dbg << "  State: " << endl << msatstate_ << endl;
    dbg << end;

    auto&& pred = msatef.getBoolVar("$CHECK$");
    s.add(pred.implies(msatquery_).asAxiom());

    {
        TRACE_BLOCK("mathsat::check");

        auto&& pred_e = pred.getExpr();
        auto&& r = s.check({pred_e});
        dbg << "Acquired result: "
            << ((r == MSAT_SAT) ? "sat" : (r == MSAT_UNSAT) ? "unsat" : "unknown")
            << endl;

        dbg << "With:" << endl;
        if (r == MSAT_SAT) {
            auto&& model = s.get_model();
            dbg << util::viewContainer(model).toVector() << endl;
            return std::make_tuple(r, util::just(model), util::nothing(), util::nothing());

        } else if (r == MSAT_UNSAT) {
            auto&& core = s.unsat_core();
            for (auto&& i = 0U; i < core.size(); ++i) dbg << core[i] << endl;
            return std::make_tuple(r, util::nothing(), util::just(core), util::nothing());

        } else {
            std::string reason{ "UNKNOWN" }; // XXX: Extraction for reason???
            dbg << reason << endl;
            return std::make_tuple(r, util::nothing(), util::nothing(), util::just(reason));
        }
    }
}

template<class TermCollection>
SatResult::model_t recollectModel(
    ExprFactory& z3ef,
    ExecutionContext& ctx,
    mathsat::Model& implModel,
    const TermCollection& vars) {
    return util::viewContainer(vars)
        .map([&](auto&& var) {
            auto&& e = SMT<MathSAT>::doit(var, z3ef, &ctx);
            auto&& solver_e = e.getExpr();

            dbgs() << "Evaluating " << solver_e << endl;

            auto&& retz3e = implModel.eval(solver_e);

            return std::make_pair(var->getName(), unlogic::undoThat(Dynamic(z3ef.unwrap(), retz3e)));
        })
        .template to<SatResult::model_t>();
}

smt::Result Solver::isViolated(
        PredicateState::Ptr query,
        PredicateState::Ptr state) {
    TRACE_FUNC;

    dbgs() << "Checking query: " << endl
           << query << endl
           << "in: " << endl
           << state << endl;

    ExecutionContext ctx{ msatef, memoryStart, memoryEnd };
    auto&& msatstate = SMT<MathSAT>::doit(state, msatef, &ctx);
    auto&& msatquery = SMT<MathSAT>::doit(query, msatef, &ctx);

    msat_result res;
    util::option<mathsat::Model> model;
    std::tie(res, model, std::ignore, std::ignore) = check(!msatquery, msatstate);

    if (res == MSAT_SAT) {
        auto&& m = model.getUnsafe(); // You shall not fail! (c)

        auto&& cex = state
            ->filterByTypes({PredicateType::PATH})
            ->filter([&](auto&& p) {
                auto&& msatp = SMT<MathSAT>::doit(p, msatef, &ctx);
                auto&& valid = m.eval(msatp.asAxiom());
                auto&& bValid = util::stringCast<bool>(valid);
                return bValid.getOrElse(false);
            });

        using namespace logging;
        dbgs() << "CEX: "
               << print_predicate_locus_on
               << cex
               << print_predicate_locus_off
               << endl;

        if (gather_msat_models.get(false) or gather_smt_models.get(false)) {
            auto&& vars = collectVariables(FactoryNest{}, query, state);

            auto&& collectedModel = recollectModel(msatef, ctx, m, vars);

            // FIXME: implement memory collection for MathSAT
            return SatResult{
                util::copy_or_share(collectedModel),
                nullptr,
                nullptr
            };
        }

        return SatResult{};
    }

    return UnsatResult{};
}

smt::Result Solver::isPathImpossible(
        PredicateState::Ptr path,
        PredicateState::Ptr state) {
    TRACE_FUNC;

    dbgs() << "Checking path: " << endl
           << path << endl
           << "in: " << endl
           << state << endl;

    ExecutionContext ctx{ msatef, memoryStart, memoryEnd };
    auto&& msatstate = SMT<MathSAT>::doit(state, msatef, &ctx);
    auto&& msatpath = SMT<MathSAT>::doit(path, msatef, &ctx);

    msat_result res;
    util::option<mathsat::Model> model;
    std::tie(res, model, std::ignore, std::ignore) = check(msatpath, msatstate);

    if (res == MSAT_SAT) {
        auto&& m = model.getUnsafe(); // You shall not fail! (c)

        if (gather_msat_models.get(false) or gather_smt_models.get(false)) {
            auto&& vars = collectVariables(FactoryNest{}, path, state);

            auto&& collectedModel = recollectModel(msatef, ctx, m, vars);

            // FIXME: implement memory collection for MathSAT
            // XXX: do we need collection for path possibility queries?
            return SatResult{
                util::copy_or_share(collectedModel),
                nullptr,
                nullptr
            };
        }

        return SatResult{};
    }

    return UnsatResult{};
}

Dynamic Solver::getInterpolant(
        PredicateState::Ptr query,
        PredicateState::Ptr body) {

    using namespace logic;

    TRACE_FUNC;

    ExecutionContext ctx{ msatef, memoryStart, memoryEnd };
    auto&& msatbody = SMT<MathSAT>::doit(body, msatef, &ctx);
    auto&& msatquery = SMT<MathSAT>::doit(query, msatef, &ctx);

    dbgs() << "Interpolating: " << endl
           << msatquery << endl
           << "in: " << endl
           << msatbody << endl;

    mathsat::ISolver s{ msatef.unwrap() };

    auto&& B = s.create_and_set_itp_group();
    s.add(msatbody.asAxiom());
    /* auto&& Q = */ s.create_and_set_itp_group();
    s.add(msatquery.asAxiom());

    {
        TRACE_BLOCK("mathsat::interpol");
        auto&& r = s.check();

        dbgs() << "Acquired result: "
               << ((r == MSAT_SAT) ? "sat" : (r == MSAT_UNSAT) ? "unsat" : "unknown")
               << endl;

        auto&& interpol = msatef.getTrue().asAxiom();

        if (r == MSAT_UNSAT) interpol = s.get_interpolant({B});

        dbgs() << "Got: " << endl
               << interpol << endl;
        return Dynamic{ msatef.unwrap(), interpol };
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
    PredicateState::Ptr pathState;
    std::tie(pathState, std::ignore) = state->splitByTypes({PredicateType::PATH});

    std::vector<Term::Ptr> pathTerms;
    pathState->map([&](auto&& p) {
        if (auto&& ep = llvm::dyn_cast<EqualityPredicate>(p)) {
            pathTerms.push_back(ep->getLhv());
            return p;
        } else if (auto&& ip = llvm::dyn_cast<InequalityPredicate>(p)) {
            pathTerms.push_back(ip->getLhv());
            return p;
        } else if (llvm::dyn_cast<DefaultSwitchCasePredicate>(p)) {
            return p;
        } else {
            BYE_BYE(Predicate::Ptr, "Wrong path predicate type: " + p->toString());
        }
    });

    return pathTerms;
}

Bool probeMathSat(
        ExprFactory& msatef,
        Bool body,
        Bool query,
        const std::vector<mathsat::Expr>& args,
        const std::vector<mathsat::Expr>& pathVars) {

    using namespace logic;

    static auto&& countLimit = getCountLimit();
    static auto&& attemptLimit = getAttemptLimit();

    auto && smtCtx = msatef.unwrap();

    mathsat::DSolver d{ smtCtx };
    d.add(body.asAxiom());
    d.add(query.asAxiom());

    auto&& ms = msatef.getFalse();

    auto&& count = 0U;
    auto&& attempt = 0U;
    auto&& fullCount = 0U; // for logging

    std::vector<mathsat::Expr> dvrs{ args };
    dvrs.insert(dvrs.end(), pathVars.begin(), pathVars.end());

    while (count < countLimit && attempt < attemptLimit) {
        auto&& models = d.diversify_unsafe(dvrs, args, countLimit * 2);

        for (auto&& m : models) {
            ++fullCount;    // for logging

            mathsat::ISolver s{ msatef.unwrap() };
            s.create_and_set_itp_group();
            s.add(body.asAxiom());
            s.add((not query).asAxiom());
            s.add(m);

            if (MSAT_SAT == s.check()) continue;

            ms = ms || Bool(smtCtx, m);
            ++count;

            if (count >= countLimit) break;
        }

        ++attempt;
    }

    dbgs() << "Attempts: " << attempt << endl
           << "Count: " << fullCount << endl;

    return ms;
}

Bool probeZ3(ExprFactory& msatef,
             unsigned long long memoryStart,
             unsigned long long memoryEnd,
             PredicateState::Ptr body,
             PredicateState::Ptr query,
             const std::vector<Term::Ptr>& args,
             const std::vector<Term::Ptr>& pathVars) {
    std::vector<Term::Ptr> dvrs{ args };
    dvrs.insert(dvrs.end(), pathVars.begin(), pathVars.end());

    z3_::ExprFactory z3ef;
    z3_::Solver z3solver{ z3ef, memoryStart, memoryEnd };
    auto&& modelState = z3solver.probeModels(body, query, dvrs, args);

    ExecutionContext ctx{ msatef, memoryStart, memoryEnd };
    return SMT<MathSAT>::doit(modelState, msatef, &ctx);
}

Dynamic Solver::getSummary(
        const std::vector<Term::Ptr>& args,
        PredicateState::Ptr query,
        PredicateState::Ptr body) {

    using namespace logic;

    TRACE_FUNC;

    ExecutionContext ctx{ msatef, memoryStart, memoryEnd };
    auto&& msatbody = SMT<MathSAT>::doit(body, msatef, &ctx);
    auto&& msatquery = SMT<MathSAT>::doit(query, msatef, &ctx);

    dbgs() << "Summarizing: " << endl
           << msatquery << endl
           << "in: " << endl
           << msatbody << endl;

    mathsat::ISolver s{ msatef.unwrap() };

    auto&& B = s.create_and_set_itp_group();
    s.add(msatbody.asAxiom());
    auto&& Q = s.create_and_set_itp_group();
    s.add(( not msatquery).asAxiom());

    auto&& toExpr = [&](auto&& term) {
            ExecutionContext ctx{ msatef, memoryStart, memoryEnd };
            return SMT<MathSAT>::doit(term, msatef, &ctx).getExpr();
        };

    auto&& argExprs = util::viewContainer(args).map(toExpr).toVector();
    auto&& pathVars = getPathVars(body);
    auto&& pathExprs = util::viewContainer(pathVars).map(toExpr).toVector();

    {
        TRACE_BLOCK("mathsat::summarize");
        auto&& r = s.check();

        dbgs() << "Acquired result: "
               << ((r == MSAT_SAT) ? "sat" : (r == MSAT_UNSAT) ? "unsat" : "unknown")
               << endl;

        auto&& interpol = msatef.getTrue().asAxiom();

        if (r == MSAT_UNSAT) {
            interpol = s.get_interpolant({B});

        } else if (r == MSAT_SAT && model_sampling.get(true)) {
            std::ostringstream dbgStr;

            dbgStr << "Model:" << std::endl;
            for (auto&& me : s.get_model()) {
                dbgStr << me.term.decl().name() << " = " << me.value.decl().name() << std::endl;
            }
            dbgs() << dbgStr.str();

            auto&& ms = msatef.getTrue();
            if ("mathsat" == sampling_solver.get("mathsat")) {
                ms = probeMathSat(msatef, msatbody, msatquery, argExprs, pathExprs);
            } else { // Z3
                ms = probeZ3(msatef, memoryStart, memoryEnd, body, query, args, pathVars);
            }

            dbgs() << "Probes: " << endl
                   << ms << endl;

            s.set_itp_group(Q);
            s.add(ms.asAxiom());

            r = s.check();
            if (r == MSAT_UNSAT) interpol = s.get_interpolant({B});
            else dbgs() << "Oops, got MSAT_SAT for (B && not Q && models)..." << endl;
        }

        dbgs() << "Got: " << endl
               << interpol << endl;
        return Dynamic{ msatef.unwrap(), interpol };
    }
}

Dynamic Solver::getContract(
        const std::vector<Term::Ptr>& args,
        PredicateState::Ptr query,
        PredicateState::Ptr body) {

    using namespace logic;

    TRACE_FUNC;

    ExecutionContext ctx{ msatef, memoryStart, memoryEnd };
    auto&& msatbody = SMT<MathSAT>::doit(body, msatef, &ctx);
    auto&& msatquery = SMT<MathSAT>::doit(query, msatef, &ctx);

    dbgs() << "Generating contract for: " << endl
           << msatquery << endl
           << "in: " << endl
           << msatbody << endl;

    mathsat::ISolver s{ msatef.unwrap() };

    auto&& B = s.create_and_set_itp_group();

    /* auto&& Q = */ s.create_and_set_itp_group();
    s.add(msatbody.asAxiom());
    s.add(( not msatquery).asAxiom());

    auto&& toExpr = [&](auto&& term) {
        ExecutionContext ctx{ msatef, memoryStart, memoryEnd };
        return SMT<MathSAT>::doit(term, msatef, &ctx).getExpr();
    };

    auto&& argExprs = util::viewContainer(args).map(toExpr).toVector();
    auto&& pathVars = getPathVars(body);
    auto&& pathExprs = util::viewContainer(pathVars).map(toExpr).toVector();

    {
        TRACE_BLOCK("mathsat::contract");
        auto&& r = s.check();

        dbgs() << "Acquired result: "
               << ((r == MSAT_SAT) ? "sat" : (r == MSAT_UNSAT) ? "unsat" : "unknown")
               << endl;

        auto&& interpol = msatef.getTrue().asAxiom();

        if (r == MSAT_UNSAT) {
            BYE_BYE(Dynamic, "No contract exists for UNSAT formula");

        } else if (r == MSAT_SAT) {

            auto&& ms = msatef.getTrue();
            if ("mathsat" == sampling_solver.get("mathsat")) {
                ms = probeMathSat(msatef, msatbody, msatquery, argExprs, pathExprs);
            } else { // Z3
                ms = probeZ3(msatef, memoryStart, memoryEnd, body, query, args, pathVars);
            }

            dbgs() << "Probes: " << endl
                   << ms << endl;

            s.set_itp_group(B);
            s.add(ms.asAxiom());

            r = s.check();
            if (r == MSAT_UNSAT) interpol = s.get_interpolant({B});
            else dbgs() << "Oops, got MSAT_SAT for (B && not Q && models)..." << endl;

        }

        dbgs() << "Got: " << endl
               << interpol << endl;
        return Dynamic{ msatef.unwrap(), interpol };
    }
}

} // namespace mathsat_
} // namespace borealis

#include "Util/unmacros.h"
