/*
 * Z3Solver.cpp
 *
 *  Created on: Oct 31, 2012
 *      Author: ice-phoenix
 */

#include <fstream>

#include "Config/config.h"
#include "Factory/Nest.h"
#include "Logging/tracer.hpp"
#include "State/DeltaDebugger.h"
#include "State/PredicateStateBuilder.h"
#include "State/Transformer/PointerCollector.h"
#include "State/Transformer/VariableCollector.h"
#include "SMT/Z3/Divers.h"
#include "SMT/Z3/Logic.hpp"
#include "SMT/Z3/Solver.h"
#include "SMT/Z3/Tactics.h"
#include "SMT/Z3/Unlogic/Unlogic.h"
#include "SMT/Z3/Z3.h"
#include <Util/cache.hpp>
#include "Util/uuid.h"

#include "Util/macros.h"

namespace borealis {
namespace z3_ {

using namespace borealis::smt;

static borealis::config::IntConfigEntry force_timeout("z3", "force-timeout");
static borealis::config::BoolConfigEntry simplify_print("z3", "print-simplified");
static borealis::config::BoolConfigEntry log_formulae("z3", "log-formulae");

static borealis::config::StringConfigEntry dump_smt2_state{ "output", "dump-smt2-states" };
static borealis::config::StringConfigEntry dump_unknown_smt2_state{ "output", "dump-unknown-smt2-states" };

static config::BoolConfigEntry gather_smt_models("analysis", "collect-models");
static config::BoolConfigEntry gather_z3_models("analysis", "collect-z3-models");

static config::BoolConfigEntry sanity_check("analysis", "sanity-check");
static config::IntConfigEntry sanity_check_timeout("analysis", "sanity-check-timeout");

Solver::Solver(ExprFactory& z3ef, unsigned long long memoryStart, unsigned long long memoryEnd) :
        z3ef(z3ef), memoryStart(memoryStart), memoryEnd(memoryEnd) {}

z3::tactic Solver::tactics(unsigned int timeout) {
    auto&& c = z3ef.unwrap();

    z3::params main_p{ c };
    main_p.set("elim_and", true);
    main_p.set("sort_store", true);

    auto&& main = Tactics::load().build(c);

    auto&& st = z3::with(main, main_p);

    return z3::try_for(st, timeout);
}

// TODO: Move this stuff to ExecutionContext???
static impl_::z3_set_instance<z3::expr> uniqueAxioms(const ExecutionContext& ctx) {
    impl_::z3_set_instance<z3::expr> ret;
    ctx.getAxioms().foreach(APPLY(ret.insert));
    return std::move(ret);
}

Solver::check_result Solver::check(
        const Bool& z3query_,
        const Bool& z3state_,
        const ExecutionContext& ctx) {

    using namespace logic;

    TRACE_FUNC;

    auto&& s = tactics(force_timeout.get(0)).mk_solver();

    auto&& wtf = logging::wtf();

    auto&& z3state = useProactiveSimplify.get(false) ? z3state_.simplify() : z3state_;
    auto&& z3query = useProactiveSimplify.get(false) ? z3query_.simplify() : z3query_;
    auto&& axioms = uniqueAxioms(ctx);

    dbgs() << "! adding axioms started" << endl;
    s.add(z3state.asAxiom());
    util::viewContainer(axioms).foreach(APPLY(s.add));
    dbgs() << "! adding axioms finished" << endl;

    dbgs() << "! adding query started" << endl;
    s.add(z3query.getAxiom());
    dbgs() << "! adding query finished" << endl;

    if (log_formulae.get(true)) {
        dbgs() << "! printing stuff started" << endl;

        auto&& dbg = dbgs();
        dbg << "  Query: " << endl << (simplify_print.get(false) ? z3query.simplify() : z3query) << endl;
        dbg << "  State: " << endl << (simplify_print.get(false) ? z3state.simplify() : z3state) << endl;
        dbg << "  Axioms: " << endl;

        for (auto&& ax: axioms)
            dbg << (simplify_print.get(false) ? ax.simplify() : ax) << endl;

        dbg << end;

        dbgs() << "! printing stuff finished" << endl;
    }

    auto&& pred = z3ef.getBoolVar("$CHECK$");
    s.add(pred.implies(z3query).getExpr());

    if (auto&& dump_dir = dump_smt2_state.get()) {
        auto&& uuid = UUID::generate();
        auto&& smtlib2_state = s.to_smt2();

        std::ofstream dump{ dump_dir.getUnsafe() + "/" + uuid.unparsed() + ".smt2" };

        if (dump) {
            dump << smtlib2_state << std::endl;
        } else {
            wtf << "Could not dump Z3 state to: " << dump_dir.getUnsafe() << endl;
        }
    }

    {
        TRACE_BLOCK("z3::check");

        dbgs() << "! z3 started" << endl;
        auto&& pred_e = pred.getExpr();
        auto&& r = s.check(1, &pred_e);
        dbgs() << "! z3 finished" << endl;

        auto&& dbg = dbgs();

        dbg << "Acquired result: "
            << ((r == z3::sat) ? "sat" : (r == z3::unsat) ? "unsat" : "unknown")
            << endl;

        dbg << "With:" << endl;
        if (r == z3::sat) {
            auto&& model = s.get_model();

            auto&& sorted_consts = util::range(0U, model.num_consts())
                .map(APPLY(model.get_const_decl))
                .toVector();
            std::sort(sorted_consts.begin(), sorted_consts.end(),
                      [](auto&& a, auto&& b) { return util::toString(a.name()) < util::toString(b.name()); });
            for (auto&& e : sorted_consts) dbg << e << endl << "  " << model.get_const_interp(e) << endl;

            auto&& sorted_funcs = util::range(0U, model.num_funcs())
                .map(APPLY(model.get_func_decl))
                .toVector();
            std::sort(sorted_funcs.begin(), sorted_funcs.end(),
                      [](auto&& a, auto&& b) { return util::toString(a.name()) < util::toString(b.name()); });
            for (auto&& e : sorted_funcs) dbg << e << endl << "  " << model.get_func_interp(e) << endl;

            return std::make_tuple(r, util::just(model), util::nothing(), util::nothing());

        } else if (r == z3::unsat) {
            auto&& core = s.unsat_core();
            dbg << core << endl;
            return std::make_tuple(r, util::nothing(), util::just(core), util::nothing());

        } else { // z3::unknown
            if(auto&& dump_dir = dump_unknown_smt2_state.get()) {
                auto&& uuid = UUID::generate();
                auto&& smtlib2_state = s.to_smt2();

                std::ofstream dump{ dump_dir.getUnsafe() + "/" + uuid.unparsed() + ".smt2" };

                if (dump) {
                    dump << smtlib2_state << std::endl;
                } else {
                    wtf << "Could not dump Z3 state to: " << dump_dir.getUnsafe() << endl;
                }
            }

            auto&& reason = s.reason_unknown();
            dbg << reason << endl;
            return std::make_tuple(r, util::nothing(), util::nothing(), util::just(reason));
        }
    }
}

template<class TermCollection>
SatResult::model_t recollectModel(
    ExprFactory& z3ef,
    ExecutionContext& ctx,
    z3::model& implModel,
    const TermCollection& vars) {
    TRACE_FUNC
    USING_SMT_LOGIC(Z3)

    return util::viewContainer(vars)
        .map([&](auto&& var) {
            auto&& e = SMT<Z3>::doit(var, z3ef, &ctx);
            auto&& z3e = e.getExpr();

            dbgs() << "Evaluating " << z3e << endl;

            auto&& retz3e = implModel.eval(z3e, true);

            return std::make_pair(var->getName(), unlogic::undoThat(Dynamic(z3ef.unwrap(), retz3e)));
        })
        .template to<SatResult::model_t>();
}

template<class TermCollection>
std::pair<SatResult::memory_shape_t, SatResult::memory_shape_t> recollectMemory(
    ExprFactory& z3ef,
    ExecutionContext& ctx,
    z3::model& implModel,
    const TermCollection& ptrs) {
    TRACE_FUNC
    USING_SMT_LOGIC(Z3)

    SatResult::memory_shape_t retStart;
    SatResult::memory_shape_t retFinal;

    if (ptrs.empty()) return { retStart, retFinal };

    auto&& startMem = ctx.getInitialMemoryContents();
    auto&& finalMem = ctx.getCurrentMemoryContents();

    for (auto&& ptr: ptrs) {
        Z3::Pointer eptr = SMT<Z3>::doit(ptr, z3ef, &ctx);

        auto&& startV = startMem.select(eptr, z3ef.sizeForType(TypeUtils::getPointerElementType(ptr->getType())));
        auto&& finalV = finalMem.select(eptr, z3ef.sizeForType(TypeUtils::getPointerElementType(ptr->getType())));

        auto&& modelPtr = implModel.eval(eptr.getExpr(), true);
        auto&& modelStartV = implModel.eval(startV.getExpr(), true);
        auto&& modelFinalV = implModel.eval(finalV.getExpr(), true);

        auto&& undonePtr = unlogic::undoThat(Dynamic(z3ef.unwrap(), modelPtr));
        ASSERTC(llvm::isa<OpaqueIntConstantTerm>(undonePtr));
        auto&& actualPtrValue = llvm::cast<OpaqueIntConstantTerm>(undonePtr)->getValue();

        retStart[actualPtrValue] = unlogic::undoThat(Dynamic(z3ef.unwrap(), modelStartV));
        retFinal[actualPtrValue] = unlogic::undoThat(Dynamic(z3ef.unwrap(), modelFinalV));
    }

    return { std::move(retStart), std::move(retFinal) };
}

Result Solver::isViolated(
        PredicateState::Ptr query,
        PredicateState::Ptr state) {

    using namespace logic;

    TRACE_FUNC;

    static config::BoolConfigEntry logQueries("output", "smt-query-logging");
    bool noQueryLogging = not logQueries.get(false);
    if(!noQueryLogging)
        dbgs() << "Checking query: " << endl
               << query << endl
               << "in: " << endl
               << state << endl;

    // XXX: measure if it really helps
    /* thread_local static util::cache<
        std::tuple<decltype(memoryStart), decltype(memoryEnd), decltype(state)>,
        std::tuple<ExecutionContext, Bool>
    > cacher */
    auto cacher = [&](auto&& membounds) {
        //thread_local static auto&& z3ef = this->z3ef;
        unsigned long long memoryStart, memoryEnd;
        PredicateState::Ptr state;
        std::tie(memoryStart, memoryEnd, state) = membounds;
        ExecutionContext ctx{ z3ef, memoryStart, memoryEnd };
        dbgs() << "! state conversion started" << endl;
        auto&& z3state = SMT<Z3>::doit(state, z3ef, &ctx);
        dbgs() << "! state conversion finished" << endl;
        return std::make_tuple(ctx, z3state);
    };

    auto&& pr = cacher( std::make_tuple(memoryStart, memoryEnd, state) );
    auto&& ctx = std::get<0>(pr);
    auto&& z3state = std::get<1>(pr);

    dbgs() << "! query conversion started" << endl;
    auto&& z3query = SMT<Z3>::doit(query, z3ef, &ctx);
    dbgs() << "! query conversion finished" << endl;

    if (sanity_check.get(false)) {
        TRACE_BLOCK("z3::sanity_check");
        auto&& ss = tactics(sanity_check_timeout.get(5) * 1000).mk_solver();
        util::viewContainer(uniqueAxioms(ctx)).foreach(APPLY(ss.add));
        ss.add(z3state.asAxiom());

        auto&& dbg = dbgs();

        dbg << "Checking state for sanity... ";
        auto&& r = ss.check();
        if (z3::sat != r) {
            dbg << (z3::unsat == r ? "FAILED" : "TIMEOUT") << endl;

            logging::wtf() << "Sanity check failed for: " << state << endl;
            auto&& dd = DeltaDebugger{ z3ef }.reduce(state);
            logging::wtf() << "Delta debugged to: " << dd << endl;

        } else {
            dbg << "OK" << endl;
        }
        dbg << end;
    }

    z3::check_result res;
    util::option<z3::model> model;
    dbgs() << "! check started" << endl;
    std::tie(res, model, std::ignore, std::ignore) = check(not z3query, z3state, ctx);
    dbgs() << "! check finished" << endl;

    if (z3::sat == res) {
        auto m = model.getUnsafe(); // You shall not fail! (c)

        auto&& cex = state
            ->filterByTypes({PredicateType::PATH})
            ->filter([&](auto&& p) {
                auto&& z3p = SMT<Z3>::doit(p, z3ef, &ctx);
                auto&& valid = m.eval(z3p.asAxiom());
                auto&& bValid = util::stringCast<bool>(valid);
                return bValid.getOrElse(false);
            });

        using namespace logging;
        dbgs() << "CEX: "
               << print_predicate_locus_on
               << cex
               << print_predicate_locus_off
               << endl;

        if (gather_z3_models.get(false) or gather_smt_models.get(false)) {
            auto&& vars = collectVariables(FactoryNest{}, query, state);
            auto&& pointers = collectPointers(FactoryNest{}, query, state);

            auto&& collectedModel = recollectModel(z3ef, ctx, m, vars);
            auto&& collectedMems = recollectMemory(z3ef, ctx, m, pointers);

            return SatResult{
                util::copy_or_share(collectedModel),
                util::copy_or_share(collectedMems.first),
                util::copy_or_share(collectedMems.second)
            };
        }

        return SatResult{};
    }

    return UnsatResult{};
}

Result Solver::isPathImpossible(
        PredicateState::Ptr path,
        PredicateState::Ptr state) {
    TRACE_FUNC;

    dbgs() << "Checking path: " << endl
           << path << endl
           << "in: " << endl
           << state << endl;

    ExecutionContext ctx{ z3ef, memoryStart, memoryEnd };
    auto z3state = SMT<Z3>::doit(state, z3ef, &ctx);
    auto z3path = SMT<Z3>::doit(path, z3ef, &ctx);

    z3::check_result res;
    util::option<z3::model> model;
    std::tie(res, model, std::ignore, std::ignore) = check(z3path, z3state, ctx);

    if (res == z3::sat) {
        auto m = model.getUnsafe(); // You shall not fail! (c)

        // XXX: Do we need model collection for path possibility queries???
        if (gather_z3_models.get(false) or gather_smt_models.get(false)) {
            auto&& vars = collectVariables(FactoryNest{}, path, state);
            auto&& pointers = collectPointers(FactoryNest{}, path, state);

            auto&& collectedModel = recollectModel(z3ef, ctx, m, vars);
            auto&& collectedMems = recollectMemory(z3ef, ctx, m, pointers);

            return SatResult{
                util::copy_or_share(collectedModel),
                util::copy_or_share(collectedMems.first),
                util::copy_or_share(collectedMems.second)
            };
        }

        return SatResult{};
    }

    return UnsatResult{};
}


////////////////////////////////////////////////////////////////////////////////
// Model sampling
////////////////////////////////////////////////////////////////////////////////

unsigned getCountLimit() {
    static config::ConfigEntry<int> CountLimit("summary", "sampling-count-limit");
    return CountLimit.get(16);
}

unsigned getAttemptLimit() {
    static config::ConfigEntry<int> AttemptLimit("summary", "sampling-attempt-limit");
    return AttemptLimit.get(100);
}

z3::expr model2expr(const z3::model& model,
                    const std::vector<z3::expr>& collectibles) {
    auto&& valuation = model.ctx().bool_val(true);
    for (auto&& c : collectibles) {
        auto&& val = model.eval(c);
        valuation = valuation && (c == val);
    }
    return valuation;
}

PredicateState::Ptr model2state(const z3::model& model,
                                const std::vector<Term::Ptr>& collectibles,
                                const std::vector<z3::expr>& z3collects) {
    USING_SMT_LOGIC(Z3)

    FactoryNest FN;
    auto&& PSB = FN.State * FN.State->Basic();
    for (auto&& zipped : util::viewContainer(collectibles) ^ util::viewContainer(z3collects)) {
        auto&& val = model.eval(zipped.second, true);
        PSB += FN.Predicate->getEqualityPredicate(
            zipped.first,
            z3_::unlogic::undoThat(Dynamic(val.ctx(), val))
        );
    }
    return PSB();
}

PredicateState::Ptr Solver::probeModels(
        PredicateState::Ptr body,
        PredicateState::Ptr query,
        const std::vector<Term::Ptr>& diversifiers,
        const std::vector<Term::Ptr>& collectibles) {

    TRACE_FUNC

    static auto countLimit = getCountLimit();
    static auto attemptLimit = getAttemptLimit();

    ExecutionContext ctx{ z3ef, memoryStart, memoryEnd };
    auto&& z3body = SMT<Z3>::doit(body, z3ef, &ctx);
    auto&& z3query = SMT<Z3>::doit(query, z3ef, &ctx);

    auto&& t2e = [&](auto&& terms) {
        return util::viewContainer(terms)
            .map([&](auto&& t) {
                return SMT<Z3>::doit(t, z3ef, &ctx).getExpr();
            })
            .toVector();
    };

    auto&& z3divers = t2e(diversifiers);
    auto&& z3collects = t2e(collectibles);

    auto&& solver = tactics(force_timeout.get(0)).mk_solver();
    solver.add(z3body.asAxiom());
    solver.add(z3query.asAxiom());
    util::viewContainer(uniqueAxioms(ctx)).foreach(APPLY(solver.add));

    FactoryNest FN;
    std::vector<PredicateState::Ptr> states;
    states.reserve(countLimit);

    auto&& count = 0U;
    auto&& attempt = 0U;
    auto&& fullCount = 0U; // for logging

    while (count < countLimit && attempt < attemptLimit) {
        auto&& models = z3::diversify_unsafe(solver, z3divers, countLimit * 2);

        for (auto&& model : models) {
            ++fullCount; // for logging

            auto&& z3model = model2expr(model, z3collects);
            auto&& stateModel = model2state(model, collectibles, z3collects);

            auto&& usolver = tactics(force_timeout.get(0)).mk_solver();
            usolver.add(z3body.asAxiom());
            usolver.add((not z3query).asAxiom());
            usolver.add( z3model );

            if (z3::sat == usolver.check()) continue;

            states.push_back(stateModel);
            ++count;

            if (count >= countLimit) break;
        }

        ++attempt;
    }

    dbgs() << "Attempts: " << attempt << endl
           << "Count: " << fullCount << endl;

    return FN.State->Choice(states);
}

} // namespace z3_
} // namespace borealis

#include "Util/unmacros.h"
