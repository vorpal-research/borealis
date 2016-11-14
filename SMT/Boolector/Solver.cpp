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
#include "State/Transformer/PropertyTargetCollector.h"
#include "SMT/Boolector/Logic.hpp"
#include "SMT/Boolector/Unlogic/Unlogic.h"
#include "SMT/Boolector/Solver.h"
#include "SMT/Boolector/Boolector.h"
#include "Util/cache.hpp"
#include "Util/uuid.h"

#include "Util/macros.h"

namespace borealis {
namespace boolector_ {

using namespace borealis::smt;

static borealis::config::IntConfigEntry force_timeout("boolector", "force-timeout");
static borealis::config::BoolConfigEntry simplify_print("boolector", "print-simplified");
static borealis::config::BoolConfigEntry log_formulae("boolector", "log-formulae");

static borealis::config::StringConfigEntry dump_smt2_state{ "output", "dump-smt2-states" };
static borealis::config::StringConfigEntry dump_unknown_smt2_state{ "output", "dump-unknown-smt2-states" };

static config::BoolConfigEntry gather_smt_models("analysis", "collect-models");
static config::BoolConfigEntry gather_boolector_models("analysis", "collect-boolector-models");

static config::BoolConfigEntry sanity_check("analysis", "sanity-check");
static config::IntConfigEntry sanity_check_timeout("analysis", "sanity-check-timeout");

Solver::Solver(ExprFactory& bef, unsigned long long memoryStart, unsigned long long memoryEnd) :
        bef(bef), memoryStart(memoryStart), memoryEnd(memoryEnd) {}

static std::unordered_set<boolectorpp::node, std::hash<boolectorpp::node>, BoolectorEngine::equality> uniqueAxioms(const ExecutionContext& ctx) {
    std::unordered_set<boolectorpp::node, std::hash<boolectorpp::node>, BoolectorEngine::equality> ret;
    ctx.getAxioms().foreach(APPLY(ret.insert));
    return std::move(ret);
}

Solver::check_result Solver::check(
        const Bool& query,
        const Bool& state,
        const ExecutionContext& ctx) {

    using namespace logic;

    TRACE_FUNC;

    auto&& bctx = bef.unwrap();
    boolector_set_opt(bctx, "model_gen", 2);

    auto&& axioms = uniqueAxioms(ctx);

    dbgs() << "! adding axioms started" << endl;
    for(auto&& ax : axioms) {
        boolector_assert(bctx, ax);
    }
    boolector_assert(bctx, state.asAxiom());
    dbgs() << "! adding axioms finished" << endl;

    dbgs() << "! adding query started" << endl;
    boolector_assert(bctx, query.getAxiom());
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

    boolector_assert(bctx, query.getExpr());
    {
        TRACE_BLOCK("boolector::check");

        auto dbg = dbgs();

        // boolector_dump_smt2(bctx, stderr);

        auto res = boolector_sat(bctx);

        dbg << "Acquired result: "
            << (res == BOOLECTOR_SAT? "sat" : (res == BOOLECTOR_UNSAT) ? "unsat" : "unknown")
            << endl;

        dbg << "With:" << endl;
        if (res == BOOLECTOR_SAT) {

            return std::make_tuple(res, static_cast<boolectorpp::context*>(bctx), util::nothing(), util::nothing());

        } else if (res == BOOLECTOR_UNSAT) {
            // TODO; boolector does not support unsat cores
            return std::make_tuple(res, nullptr, util::nothing(), util::nothing());

        } else {
            // implement reason
            return std::make_tuple(res, nullptr, util::nothing(), util::nothing());
        }
    }
}

template<class TermCollection>
Model::assignments_t recollectModel(
    const FactoryNest& FN,
    ExprFactory& ef,
    ExecutionContext& ctx,
    Btor* /* smt */,
    const TermCollection& vars) {
    TRACE_FUNC
    USING_SMT_LOGIC(Boolector)

    return util::viewContainer(vars)
        .map([&](auto&& var) {
            auto&& e = SMT<Boolector>::doit(var, ef, &ctx);
            dbgs() << "Evaluating " << e << endl;

            return std::make_pair(var, unlogic::undoThat(FN, var, e));
        })
        .template to<Model::assignments_t>();
}

template<class TermCollection>
void recollectMemory(
    Model& model,
    ExprFactory& ef,
    ExecutionContext& ctx,
    Btor* /* smt */,
    const TermCollection& ptrs) {
    TRACE_FUNC
    USING_SMT_LOGIC(Boolector)

    if (ptrs.empty()) return;

    auto&& FN = model.getFactoryNest();
    TermBuilder TB(FN.Term, nullptr);

    for (auto&& ptr: ptrs) {
        size_t memspace = 0;
        if(auto&& pt = llvm::dyn_cast<type::Pointer>(ptr->getType())) {
            memspace = pt->getMemspace();
        }
        auto&& startMem = ctx.getInitialMemoryContents(memspace);
        auto&& finalMem = ctx.getCurrentMemoryContents(memspace);

        auto&& startBounds = ctx.getCurrentGepBounds(memspace);
        // auto&& finalBounds = ctx.getCurrentGepBounds(memspace);

        Boolector::Pointer eptr = SMT<Boolector>::doit(ptr, ef, &ctx);

        auto&& startV = startMem.select(eptr, ef.sizeForType(TypeUtils::getPointerElementType(ptr->getType())));
        auto&& finalV = finalMem.select(eptr, ef.sizeForType(TypeUtils::getPointerElementType(ptr->getType())));

        auto&& startBound = startBounds[eptr];
        auto&& finalBound = startBounds[eptr];

        auto&& modelPtr = (eptr.getExpr());
        auto&& modelStartV = (startV.getExpr());
        auto&& modelFinalV = (finalV.getExpr());

        auto&& modelStartBound = (startBound.getExpr());
        auto&& modelFinalBound = (finalBound.getExpr());

        auto&& undonePtr = unlogic::undoThat(model.getFactoryNest(), ptr, Dynamic(ef.unwrap(), modelPtr));

        model.getMemories()[memspace].getInitialMemoryShape()[undonePtr]
            = unlogic::undoThat(FN, *TB(undonePtr), Dynamic(ef.unwrap(), modelStartV));
        model.getMemories()[memspace].getFinalMemoryShape()[undonePtr]
            = unlogic::undoThat(FN, *TB(undonePtr), Dynamic(ef.unwrap(), modelFinalV));

        model.getBounds()[memspace].getInitialMemoryShape()[undonePtr]
            = unlogic::undoThat(FN, TB(undonePtr).bound(), Dynamic(ef.unwrap(), modelStartBound));
        model.getBounds()[memspace].getFinalMemoryShape()[undonePtr]
            = unlogic::undoThat(FN, TB(undonePtr).bound(), Dynamic(ef.unwrap(), modelFinalBound));

    }
}


template<class TermCollection>
void recollectProperties(
    Model& model,
    ExprFactory& ef,
    ExecutionContext& ctx,
    Btor* /* smt */,
    const TermCollection& props) {
    TRACE_FUNC
    USING_SMT_LOGIC(Boolector)

    if (props.empty()) return;

    auto&& FN = model.getFactoryNest();
    TermBuilder TB(FN.Term, nullptr);

    for (auto&& kv: props) {
        auto&& ptr = kv.second;
        auto&& pname = kv.first;

        auto&&  arr = ctx.getCurrentProperties(pname);

        auto e = SMT<Boolector>::doit(ptr, ef, &ctx);
        auto eptr = Boolector::Pointer::forceCast(e);

        auto&& startV = arr[eptr];

        auto&& modelPtr = (eptr.getExpr());
        auto&& modelStartV = (startV.getExpr());

        auto&& undonePtr = unlogic::undoThat(model.getFactoryNest(), ptr, Dynamic(ef.unwrap(), modelPtr));

        model.getProperties()[pname].getFinalMemoryShape()[undonePtr]
            = unlogic::undoThat(FN, FN.Term->getIntTerm(0, FN.Type->getInteger()), Dynamic(ef.unwrap(), modelStartV));
    }
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
        ExecutionContext ctx{ bef, memoryStart, memoryEnd };
        dbgs() << "! state conversion started" << endl;
        auto&& smtstate = SMT<Boolector>::doit(state, bef, &ctx);
        dbgs() << "! state conversion finished" << endl;
        return std::make_tuple(ctx, smtstate);
    };

    auto&& pr = cacher( std::make_tuple(memoryStart, memoryEnd, state) );
    auto&& ctx = std::get<0>(pr);
    auto&& smtstate = std::get<1>(pr);

    dbgs() << "! query conversion started" << endl;
    auto&& smtquery = SMT<Boolector>::doit(query, bef, &ctx);
    dbgs() << "! query conversion finished" << endl;

    int res;
    boolectorpp::context* cmodel;
    dbgs() << "! check started" << endl;
    std::tie(res, cmodel, std::ignore, std::ignore) = check(not smtquery, smtstate, ctx);
    dbgs() << "! check finished" << endl;

    if (res == BOOLECTOR_SAT) {
        // XXX: Do we need model collection for path possibility queries???
        if (gather_boolector_models.get(false) or gather_smt_models.get(false)) {

            FactoryNest FN;
            auto&& vars = collectVariables(FN, query, state);
            auto&& pointers = collectPointers(FN, query, state);
            auto&& props = collectPropertyTargets(FN, query, state);

            auto&& model = std::make_shared<Model>(FN);

            model->getAssignments() = recollectModel(FN, bef, ctx, *cmodel, vars);
            recollectMemory(*model, bef, ctx, *cmodel, pointers);
            recollectProperties(*model, bef, ctx, *cmodel, props);

            return SatResult{model};
        }

        return SatResult{};
    }

    return UnsatResult{};
}

void Solver::interrupt() {
}

} // namespace boolector_
} // namespace borealis

#include "Util/unmacros.h"
