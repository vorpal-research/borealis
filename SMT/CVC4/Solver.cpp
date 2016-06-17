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
#include "SMT/CVC4/Logic.hpp"
#include "SMT/CVC4/Unlogic/Unlogic.h"
#include "SMT/CVC4/Solver.h"
#include "SMT/CVC4/CVC4.h"
#include "Util/cache.hpp"
#include "Util/uuid.h"

#include "Util/macros.h"

namespace borealis {
namespace cvc4_ {

using namespace borealis::smt;

static borealis::config::IntConfigEntry force_timeout("cvc4", "force-timeout");
static borealis::config::BoolConfigEntry simplify_print("cvc4", "print-simplified");
static borealis::config::BoolConfigEntry log_formulae("cvc4", "log-formulae");

static borealis::config::StringConfigEntry dump_smt2_state{ "output", "dump-smt2-states" };
static borealis::config::StringConfigEntry dump_unknown_smt2_state{ "output", "dump-unknown-smt2-states" };

static config::BoolConfigEntry gather_smt_models("analysis", "collect-models");
static config::BoolConfigEntry gather_cvc4_models("analysis", "collect-cvc4-models");

static config::BoolConfigEntry sanity_check("analysis", "sanity-check");
static config::IntConfigEntry sanity_check_timeout("analysis", "sanity-check-timeout");

Solver::Solver(ExprFactory& cvc4ef, unsigned long long memoryStart, unsigned long long memoryEnd) :
        cvc4ef(cvc4ef), memoryStart(memoryStart), memoryEnd(memoryEnd) {}


static std::unordered_set<::CVC4::Expr, std::hash<::CVC4::Expr>, CVC4Engine::equality> uniqueAxioms(const ExecutionContext& ctx) {
    std::unordered_set<::CVC4::Expr, std::hash<::CVC4::Expr>, CVC4Engine::equality> ret;
    ctx.getAxioms().foreach(APPLY(ret.insert));
    return std::move(ret);
}

Solver::check_result Solver::check(
        const Bool& query,
        const Bool& state,
        const ExecutionContext& ctx) {

    using namespace logic;
    using namespace ::CVC4;

    TRACE_FUNC;

    currentEngine = std::make_shared<::CVC4::SmtEngine>(&cvc4ef.unwrap().em);
    ON_SCOPE_EXIT(currentEngine.reset());

    auto s = currentEngine;
    s->setLogic("QF_AUFBV");
    s->setOption("produce-models", true);
    s->setOption("output-language", "smt2");
    s->setOption("interactive-mode", true);

    for(auto&& tl : force_timeout.get()) s->setTimeLimit(tl);

    auto&& wtf = logging::wtf();

    auto&& axioms = uniqueAxioms(ctx);

    std::vector<::CVC4::Expr> assertions;

    ON_SCOPE_EXIT(
        if(std::uncaught_exception()) {
            std::cerr << "CVC4 threw:" << std::endl;
            std::cerr << "Assertions:" << std::endl;
            for(auto&& as : assertions) {
                std::cerr << as << std::endl;
            }
        }
    )


    dbgs() << "! adding axioms started" << endl;
    assertions.insert(assertions.begin(), axioms.begin(), axioms.end());
    assertions.push_back(state.asAxiom());
    dbgs() << "! adding axioms finished" << endl;

    dbgs() << "! adding query started" << endl;
    assertions.push_back(query.getAxiom());
    dbgs() << "! adding query finished" << endl;

    if (log_formulae.get(true)) {
        dbgs() << "! printing stuff started" << endl;

        auto&& dbg = dbgs();
        dbg << "  Query: " << endl << query << endl;
        dbg << "  State: " << endl << state << endl;
        dbg << "  Axioms: " << endl;

        for (auto&& ax: axioms)
            dbg << ax << endl;

        dbg << end;

        dbgs() << "! printing stuff finished" << endl;
    }

    auto&& pred = cvc4ef.getBoolVar("$CHECK$");
    assertions.push_back(pred.implies(query).getExpr());

    for(auto&& ass : assertions) s->assertFormula(ass);
    //s.assertFormula(cvc4ef.unwrap().mkExpr(::CVC4::kind::AND, assertions));



    {
        TRACE_BLOCK("cvc4::check");

        dbgs() << "! cvc4 started" << endl;
        auto&& pred_e = pred.getExpr();
        auto&& r = s->checkSat(pred_e);
        dbgs() << "! cvc4 finished" << endl;

        auto&& dbg = dbgs();

        dbg << "Acquired result: "
            << ((r.isSat() == ::CVC4::Result::SAT) ? "sat" : (r.isSat() == ::CVC4::Result::UNSAT) ? "unsat" : "unknown")
            << endl;

        dbg << "With:" << endl;
        if (r.isSat() == ::CVC4::Result::SAT) {

            return std::make_tuple(r, s, util::nothing(), util::nothing());

        } else if (r.isSat() == ::CVC4::Result::UNSAT) {
            // TODO; cvc4 does not support unsat cores
            return std::make_tuple(r, nullptr, util::nothing(), util::nothing());

        } else { // z3::unknown
            dbg << util::toString(r) << endl;
            // implement reason
            return std::make_tuple(r, nullptr, util::nothing(), util::nothing());
        }
    }
}

template<class TermCollection>
SatResult::model_t recollectModel(
    ExprFactory& ef,
    ExecutionContext& ctx,
    ::CVC4::SmtEngine& smt,
    const TermCollection& vars) {
    TRACE_FUNC
    USING_SMT_LOGIC(CVC4)

    return util::viewContainer(vars)
        .map([&](auto&& var) {
            auto&& e = SMT<CVC4>::doit(var, ef, &ctx);
            auto&& cvc4e = e.getExpr();

            dbgs() << "Evaluating " << cvc4e << endl;

            auto&& ret_e = smt.getValue(cvc4e);

            return std::make_pair(var->getName(), unlogic::undoThat(Dynamic(ef.unwrap(), ret_e)));
        })
        .template to<SatResult::model_t>();
}

template<class TermCollection>
std::pair<SatResult::memory_shape_t, SatResult::memory_shape_t> recollectMemory(
    ExprFactory& ef,
    ExecutionContext& ctx,
    ::CVC4::SmtEngine& smt,
    const TermCollection& ptrs) {
    TRACE_FUNC
    USING_SMT_LOGIC(CVC4)

    SatResult::memory_shape_t retStart;
    SatResult::memory_shape_t retFinal;

    if (ptrs.empty()) return { retStart, retFinal };

    auto&& startMem = ctx.getInitialMemoryContents();
    auto&& finalMem = ctx.getCurrentMemoryContents();

    for (auto&& ptr: ptrs) {
        CVC4::Pointer eptr = SMT<CVC4>::doit(ptr, ef, &ctx);

        auto&& startV = startMem.select(eptr, ef.sizeForType(TypeUtils::getPointerElementType(ptr->getType())));
        auto&& finalV = finalMem.select(eptr, ef.sizeForType(TypeUtils::getPointerElementType(ptr->getType())));

        auto&& modelPtr = smt.getValue(eptr.getExpr());
        auto&& modelStartV = smt.getValue(startV.getExpr());
        auto&& modelFinalV = smt.getValue(finalV.getExpr());

        auto&& undonePtr = unlogic::undoThat(Dynamic(ef.unwrap(), modelPtr));
        ASSERTC(llvm::isa<OpaqueIntConstantTerm>(undonePtr));
        auto&& actualPtrValue = llvm::cast<OpaqueIntConstantTerm>(undonePtr)->getValue();

        retStart[actualPtrValue] = unlogic::undoThat(Dynamic(ef.unwrap(), modelStartV));
        retFinal[actualPtrValue] = unlogic::undoThat(Dynamic(ef.unwrap(), modelFinalV));
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
        ExecutionContext ctx{ cvc4ef, memoryStart, memoryEnd };
        dbgs() << "! state conversion started" << endl;
        auto&& smtstate = SMT<CVC4>::doit(state, cvc4ef, &ctx);
        dbgs() << "! state conversion finished" << endl;
        return std::make_tuple(ctx, smtstate);
    };

    auto&& pr = cacher( std::make_tuple(memoryStart, memoryEnd, state) );
    auto&& ctx = std::get<0>(pr);
    auto&& smtstate = std::get<1>(pr);

    dbgs() << "! query conversion started" << endl;
    auto&& smtquery = SMT<CVC4>::doit(query, cvc4ef, &ctx);
    dbgs() << "! query conversion finished" << endl;

    ::CVC4::Result res;
    std::shared_ptr<::CVC4::SmtEngine> model;
    dbgs() << "! check started" << endl;
    std::tie(res, model, std::ignore, std::ignore) = check(not smtquery, smtstate, ctx);
    dbgs() << "! check finished" << endl;

    if (res.isSat() == res.SAT) {
        auto&& m = *model; // You shall not fail! (c)

        // XXX: Do we need model collection for path possibility queries???
        if (gather_cvc4_models.get(false) or gather_smt_models.get(false)) {
            auto&& vars = collectVariables(FactoryNest{}, query, state);
            auto&& pointers = collectPointers(FactoryNest{}, query, state);

            auto&& collectedModel = recollectModel(cvc4ef, ctx, m, vars);
            auto&& collectedMems = recollectMemory(cvc4ef, ctx, m, pointers);

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

    ExecutionContext ctx{ cvc4ef, memoryStart, memoryEnd };
    auto z3state = SMT<CVC4>::doit(state, cvc4ef, &ctx);
    auto z3path = SMT<CVC4>::doit(path, cvc4ef, &ctx);

    ::CVC4::Result res;

    std::tie(res, std::ignore, std::ignore, std::ignore) = check(z3path, z3state, ctx);

    if (res.isSat() == res.SAT) {
        return SatResult{};
    }

    return UnsatResult{};
}

void Solver::interrupt() {
    if(currentEngine) currentEngine->interrupt();
}

} // namespace cvc4_
} // namespace borealis

#include "Util/unmacros.h"
