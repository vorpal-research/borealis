/*
 * Z3Solver.cpp
 *
 *  Created on: Oct 31, 2012
 *      Author: ice-phoenix
 */

#include "State/Transformer/VariableCollector.h"
#include "State/Transformer/PointerCollector.h"
#include "Factory/Nest.h"
#include "Logging/tracer.hpp"
#include "State/PredicateStateBuilder.h"
#include "SMT/Z3/Divers.h"
#include "SMT/Z3/Logic.hpp"
#include "SMT/Z3/Solver.h"
#include "SMT/Z3/Unlogic/Unlogic.h"

#include "Util/macros.h"

namespace borealis {
namespace z3_ {
using namespace borealis::smt;

Solver::Solver(ExprFactory& z3ef, unsigned long long memoryStart, unsigned long long memoryEnd) :
        z3ef(z3ef), memoryStart(memoryStart), memoryEnd(memoryEnd) {}

z3::tactic Solver::tactics() {
    auto& c = z3ef.unwrap();

    auto params = z3::params(c);
    params.set("auto_config", true);
    auto smt_tactic = with(z3::tactic(c, "smt"), params);
    auto useful = /* z3::tactic(c, "reduce-bv-size") & */ z3::tactic(c, "ctx-simplify");

    return useful & smt_tactic;
}

Solver::check_result Solver::check(
        const Bool& z3query_,
        const Bool& z3state_) {

    using namespace logic;

    TRACE_FUNC;

    auto s = tactics().mk_solver();

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

template<class TermCollection>
SatResult::model_t recollectModel(ExprFactory& z3ef, ExecutionContext& ctx, z3::model implModel, const TermCollection& vars) {
    TRACE_FUNC
    return
        util::viewContainer(vars)
        .map([&](Term::Ptr var) -> std::pair<std::string, Term::Ptr> {
            auto e = SMT<Z3>::doit(var, z3ef, &ctx);
            auto z3e = logic::z3impl::getExpr(e);

            dbgs() << "Evaluating " << z3e << endl;

            auto retz3e = implModel.eval(z3e, true);

            return { var->getName(), unlogic::undoThat(retz3e) };
        })
        .template to<SatResult::model_t>();
}

template<class TermCollection>
std::pair<SatResult::memory_shape_t, SatResult::memory_shape_t> recollectMemory(
    ExprFactory& z3ef,
    ExecutionContext& ctx,
    z3::model implModel,
    const TermCollection& ptrs) {
    TRACE_FUNC

    using namespace logic::z3impl;

    SatResult::memory_shape_t retStart;
    SatResult::memory_shape_t retFinal;

    if(ptrs.empty()) return { retStart, retFinal };

    auto startMem = ctx.getInitialMemoryContents();
    auto finalMem = ctx.getCurrentMemoryContents();
    for (auto&& ptr: ptrs) {
        auto eptr = SMT<Z3>::doit(ptr, z3ef, &ctx).template to<Z3::Pointer>().getUnsafe();

        auto startV = startMem.select(eptr, z3ef.sizeForType(TypeUtils::getPointerElementType(ptr->getType())));
        auto finalV = finalMem.select(eptr, z3ef.sizeForType(TypeUtils::getPointerElementType(ptr->getType())));

        auto modelStartV = implModel.eval(getExpr(startV), true);
        auto modelFinalV = implModel.eval(getExpr(finalV), true);
        auto modelPtr = implModel.eval(getExpr(eptr), true);

        auto undonePtr = unlogic::undoThat(modelPtr);
        ASSERTC(llvm::isa<OpaqueIntConstantTerm>(undonePtr));
        auto actualPtrValue = llvm::cast<OpaqueIntConstantTerm>(undonePtr)->getValue();

        retStart[actualPtrValue] = unlogic::undoThat(modelStartV);
        retFinal[actualPtrValue] = unlogic::undoThat(modelFinalV);
    }

    return { std::move(retStart), std::move(retFinal) };
}

static config::BoolConfigEntry gatherSMTModels("analysis", "collect-models");
static config::BoolConfigEntry gatherZ3Models("analysis", "collect-z3-models");

Result Solver::isViolated(
        PredicateState::Ptr query,
        PredicateState::Ptr state) {
    TRACE_FUNC;

    dbgs() << "Checking query: " << endl
           << query << endl
           << "in: " << endl
           << state << endl;

    ExecutionContext ctx(z3ef, memoryStart, memoryEnd);
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

        if(gatherZ3Models.get(false) or gatherSMTModels.get(false)) {
            auto vars = collectVariables(FactoryNest{}, query, state);
            auto pointers = collectPointers(FactoryNest{}, query, state);

            auto collectedModel = recollectModel(z3ef, ctx, m, vars);
            auto collectedMems = recollectMemory(z3ef, ctx, m, pointers);

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

    ExecutionContext ctx(z3ef, memoryStart, memoryEnd);
    auto z3state = SMT<Z3>::doit(state, z3ef, &ctx);
    auto z3path = SMT<Z3>::doit(path, z3ef, &ctx);

    z3::check_result res;
    util::option<z3::model> model;
    std::tie(res, model, std::ignore, std::ignore) = check(z3path, z3state);

    if (res == z3::sat) {
        auto m = model.getUnsafe(); // You shall not fail! (c)

        if(gatherZ3Models.get(false) or gatherSMTModels.get(false)) {
            auto vars = collectVariables(FactoryNest{}, path, state);
            auto pointers = collectPointers(FactoryNest{}, path, state);

            auto collectedModel = recollectModel(z3ef, ctx, m, vars);
            auto collectedMems = recollectMemory(z3ef, ctx, m, pointers);

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
    auto valuation = model.ctx().bool_val(true);
    for (auto c: collectibles) {
        auto val = model.eval(c);
        valuation = valuation && (c == val);
    }
    return valuation;
}

PredicateState::Ptr model2state(const z3::model& model,
                                const std::vector<Term::Ptr>& collectibles,
                                const std::vector<z3::expr>& z3collects) {
    FactoryNest FN(nullptr);
    auto PSB = FN.State * FN.State->Basic();
    for (auto zipped: util::viewContainer(collectibles) ^ util::viewContainer(z3collects)) {
        auto val = model.eval(zipped.second, true);
        PSB += FN.Predicate->getEqualityPredicate(
                    zipped.first,
                    z3_::unlogic::undoThat(val)
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

    ExecutionContext ctx(z3ef, memoryStart, memoryEnd);
    auto z3body = SMT<Z3>::doit(body, z3ef, &ctx);
    auto z3query = SMT<Z3>::doit(query, z3ef, &ctx);

    auto t2e = [this, &ctx](const std::vector<Term::Ptr>& terms) -> std::vector<z3::expr> {
        std::vector<z3::expr> exprs;
        exprs.reserve(terms.size());
        std::transform(terms.begin(), terms.end(), std::back_inserter(exprs),
                [this, &ctx](const Term::Ptr& t) -> z3::expr {
                    return logic::z3impl::getExpr(SMT<Z3>::doit(t, z3ef, &ctx));
                }
        );
        return exprs;
    };

    auto z3divers = t2e(diversifiers);
    auto z3collects = t2e(collectibles);

    auto solver = tactics().mk_solver();
    solver.add(logic::z3impl::asAxiom(z3body));
    solver.add(logic::z3impl::asAxiom(z3query));

    FactoryNest FN(nullptr);
    std::vector<PredicateState::Ptr> states;
    states.reserve(countLimit);

    auto count = 0U;
    auto attempt = 0U;
    auto fullCount = 0U; // for logging

    while (count < countLimit && attempt < attemptLimit) {
        auto models = z3::diversify_unsafe(solver, z3divers, countLimit*2);

        for (const auto& model : models) {
            ++fullCount;    // for logging

            auto z3model = model2expr(model, z3collects);
            auto stateModel = model2state(model, collectibles, z3collects);

            auto usolver = tactics().mk_solver();
            usolver.add(logic::z3impl::asAxiom(z3body));
            usolver.add(logic::z3impl::asAxiom(not z3query));
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
