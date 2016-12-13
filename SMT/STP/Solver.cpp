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
#include "SMT/STP/Logic.hpp"
#include "SMT/STP/Unlogic/Unlogic.h"
#include "SMT/STP/Solver.h"
#include "SMT/STP/STP.h"
#include "Util/cache.hpp"
#include "Util/uuid.h"

#include "Util/macros.h"

namespace borealis {
namespace stp_ {

enum { INVALID = 0, VALID = 1, UNKNOWN = 2, TIMEOUT = 3, ERROR = 4 };
enum { SAT = 5, UNSAT = 6 };

using namespace borealis::smt;

static borealis::config::IntConfigEntry force_timeout("stp", "force-timeout");
static borealis::config::BoolConfigEntry simplify_print("stp", "print-simplified");
static borealis::config::BoolConfigEntry log_formulae("stp", "log-formulae");

static borealis::config::StringConfigEntry dump_smt2_state{ "output", "dump-smt2-states" };
static borealis::config::StringConfigEntry dump_unknown_smt2_state{ "output", "dump-unknown-smt2-states" };

static config::BoolConfigEntry gather_smt_models("analysis", "collect-models");
static config::BoolConfigEntry gather_stp_models("analysis", "collect-boolector-models");

static config::BoolConfigEntry sanity_check("analysis", "sanity-check");
static config::IntConfigEntry sanity_check_timeout("analysis", "sanity-check-timeout");

Solver::Solver(ExprFactory& bef, unsigned long long memoryStart, unsigned long long memoryEnd) :
        bef(bef), memoryStart(memoryStart), memoryEnd(memoryEnd) {}

static std::unordered_set<stppp::node, std::hash<stppp::node>, STPEngine::equality> uniqueAxioms(const ExecutionContext& ctx) {
    std::unordered_set<stppp::node, std::hash<stppp::node>, STPEngine::equality> ret;
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

    auto&& axioms = uniqueAxioms(ctx);

    dbgs() << "! adding axioms started" << endl;
    for(auto&& ax : axioms) {
        vc_assertFormula(bctx, ax);
    }
    vc_assertFormula(bctx, state.asAxiom());
    dbgs() << "! adding axioms finished" << endl;

    dbgs() << "! adding query started" << endl;
    vc_assertFormula(bctx, query.getAxiom());

    dbgs() << "! adding query finished" << endl;

    if (log_formulae.get(false)) {
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



    {
        TRACE_BLOCK("stp::check");

        auto dbg = dbgs();

        // stp_dump_smt2(bctx, stderr);
        // vc_printAsserts(bctx);
        // vc_printExpr(bctx, query.getExpr());

        auto res = vc_query(bctx, (not query).getExpr());

        dbg << "Acquired result: "
            << (res == INVALID? "sat" : (res == VALID) ? "unsat" : "unknown")
            << endl;

        dbg << "With:" << endl;
        if (res == VALID) {
            // TODO; boolector does not support unsat cores
            return std::make_tuple(UNSAT, nullptr, util::nothing(), util::nothing());
        } else if (res == INVALID) {
            return std::make_tuple(SAT, static_cast<stppp::context*>(bctx), util::nothing(), util::nothing());
        } else {
            // implement reason
            return std::make_tuple(UNKNOWN, nullptr, util::nothing(), util::nothing());
        }
    }
}

template<class TermCollection>
Model::assignments_t recollectModel(
    const FactoryNest& FN,
    ExprFactory& ef,
    ExecutionContext& ctx,
    VC /* smt */,
    const TermCollection& vars) {
    TRACE_FUNC
    USING_SMT_LOGIC(STP)

    return util::viewContainer(vars)
        .map([&](auto&& var) {
            auto&& e = SMT<STP>::doit(var, ef, &ctx);
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
    VC /* smt */,
    const TermCollection& ptrs) {
    TRACE_FUNC
    USING_SMT_LOGIC(STP)

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

        STP::Pointer eptr = SMT<STP>::doit(ptr, ef, &ctx);

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
    VC /* smt */,
    const TermCollection& props) {
    TRACE_FUNC
    USING_SMT_LOGIC(STP)

    if (props.empty()) return;

    auto&& FN = model.getFactoryNest();
    TermBuilder TB(FN.Term, nullptr);

    for (auto&& kv: props) {
        auto&& ptr = kv.second;
        auto&& pname = kv.first;

        auto&&  arr = ctx.getCurrentProperties(pname);

        auto e = SMT<STP>::doit(ptr, ef, &ctx);
        auto eptr = STP::Pointer::forceCast(e);

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
        auto&& smtstate = SMT<STP>::doit(state, bef, &ctx);
        dbgs() << "! state conversion finished" << endl;
        return std::make_tuple(ctx, smtstate);
    };

    auto&& pr = cacher( std::make_tuple(memoryStart, memoryEnd, state) );
    auto&& ctx = std::get<0>(pr);
    auto&& smtstate = std::get<1>(pr);

    dbgs() << "! query conversion started" << endl;
    auto&& smtquery = SMT<STP>::doit(query, bef, &ctx);
    dbgs() << "! query conversion finished" << endl;

    int res;
    stppp::context* cmodel;
    dbgs() << "! check started" << endl;
    std::tie(res, cmodel, std::ignore, std::ignore) = check(not smtquery, smtstate, ctx);
    dbgs() << "! check finished" << endl;

    if (res == SAT) {
        // XXX: Do we need model collection for path possibility queries???
        if (gather_stp_models.get(false) or gather_smt_models.get(false)) {

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

    if(res != UNSAT) return UnknownResult{};

    return UnsatResult{};
}

void Solver::interrupt() {
}

} // namespace stp_
} // namespace borealis

#include "Util/unmacros.h"
