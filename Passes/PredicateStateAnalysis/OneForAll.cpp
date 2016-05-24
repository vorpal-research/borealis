/*
 * OneForAll.cpp
 *
 *  Created on: Feb 14, 2013
 *      Author: ice-phoenix
 */

#include <llvm/IR/CFG.h>
#include <llvm/Support/GraphWriter.h>

#include "Logging/tracer.hpp"
#include "Passes/PredicateAnalysis/Defines.def"
#include "Passes/PredicateStateAnalysis/OneForAll.h"
#include "Passes/Tracker/SlotTrackerPass.h"
#include "State/PredicateStateBuilder.h"
#include "State/Transformer/AggregateTransformer.h"
#include "State/Transformer/CallSiteInitializer.h"
#include "State/Transformer/MemoryContextSplitter.h"
#include "State/Transformer/Retyper.h"
#include "State/Transformer/StateOptimizer.h"
#include "State/Transformer/TermSizeCalculator.h"
#include "State/Transformer/GraphBuilder.h"
#include "Util/graph.h"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

OneForAll::OneForAll() :
        ProxyFunctionPass(ID),
        NULLPTRIFY3(DT, FM, SLT) {}
OneForAll::OneForAll(llvm::Pass* pass) :
        ProxyFunctionPass(ID, pass),
        NULLPTRIFY3(DT, FM, SLT) {}

void OneForAll::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();

    AUX<llvm::DominatorTreeWrapperPass>::addRequired(AU);
    AUX<FunctionManager>::addRequiredTransitive(AU);
    AUX<SlotTrackerPass>::addRequiredTransitive(AU);
    AUX<SourceLocationTracker>::addRequiredTransitive(AU);

#define HANDLE_ANALYSIS(CLASS) \
    AUX<CLASS>::addRequiredTransitive(AU);
#include "Passes/PredicateAnalysis/Defines.def"
}

bool OneForAll::runOnFunction(llvm::Function& F) {
    init();

    DT = &GetAnalysis< llvm::DominatorTreeWrapperPass >::doit(this, F).getDomTree();
    FM = &GetAnalysis< FunctionManager >::doit(this, F);
    SLT = &GetAnalysis< SourceLocationTracker >::doit(this, F);

    FN = FactoryNest(F.getDataLayout(), GetAnalysis< SlotTrackerPass >::doit(this, F).getSlotTracker(F));

#define HANDLE_ANALYSIS(CLASS) \
    PA.push_back(static_cast<AbstractPredicateAnalysis*>(&GetAnalysis<CLASS>::doit(this, F)));
#include "Passes/PredicateAnalysis/Defines.def"

    // Register globals in our predicate state
    auto&& gState = FN.getGlobalState(&F);

    dbgs() << "Global state size: " << TermSizeCalculator::measure(gState) << endl;
    // Register requires
    auto&& requires = FM->getReq(&F);
    // Memory split requires
    auto&& mcs = MemoryContextSplitter(FN);
    auto&& splittedRequires = mcs.transform(requires);

    auto&& initialState = (
        FN.State *
        gState +
        mcs.getGeneratedPredicates() +
        splittedRequires
    )();

    // Register arguments as visited values
    for (auto&& arg : F.getArgumentList()) {
        initialState = initialState << SLT->getLocFor(&arg);
    }

    // Save initial state
    this->initialState = initialState;

    dbgs() << "Initial state size: " << TermSizeCalculator::measure(initialState) << endl;

    // Process basic blocks in topological order
    auto&& ordered = TopologicalSorter().doit(F);
    ASSERT(not ordered.empty(),
           "No topological order for: " + F.getName().str());
    ASSERT(ordered.getUnsafe().size() == F.getBasicBlockList().size(),
           "Topological order does not include all basic blocks for: " + F.getName().str());

    dbgs() << "Topological sorting for: " << F.getName() << endl;
    for (const auto* BB : ordered.getUnsafe()) {
        dbgs() << valueSummary(BB) << endl;
    }
    dbgs() << "End of topological sorting for: " << F.getName() << endl;

    for (auto* BB : ordered.getUnsafe()) {
        processBasicBlock(BB);
    }

    finalize();

    return false;
}

////////////////////////////////////////////////////////////////////////////////

void OneForAll::init() {
    AbstractPredicateStateAnalysis::init();
    basicBlockStates.clear();
    PA.clear();
}

void OneForAll::finalize() {
    AbstractPredicateStateAnalysis::finalize();

    if (PredicateStateAnalysis::OptimizeStates()) {

        dbgs() << "Optimizer started" << endl;
        StateOptimizer so{FN};

        initialState = so.transform(initialState);
        for (auto&& v : util::viewContainerValues(instructionStates)) {
            v = so.transform(v);
        }
        dbgs() << "Optimizer finished" << endl;
    }

    Retyper retyper{FN};

    initialState = retyper.transform(initialState);
    for (auto&& v : util::viewContainerValues(instructionStates)) {
        v = retyper.transform(v);
    }
}

void OneForAll::processBasicBlock(llvm::BasicBlock* BB) {
    using namespace llvm;
    using borealis::util::viewContainer;

    auto&& inState = BBM(BB);

    auto&& fMemInfo = FM->getMemoryBounds(BB->getParent());

    if (nullptr == inState) return;
    if (PredicateStateAnalysis::CheckUnreachable() and inState->isUnreachableIn(fMemInfo.first, fMemInfo.second)) return;

    for (auto&& I : viewContainer(*BB)) {

        auto&& instructionState = (FN.State * inState + PM(&I)).apply();
        instructionStates[&I] = instructionState;

        // Add ensures and summary *after* the CallInst has been processed
        if (isa<CallInst>(I)) {
            auto&& CI = cast<CallInst>(I);

            auto&& callState = (
                FN.State *
                FM->getBdy(CI, FN) +
                FM->getEns(CI, FN)
            ).apply();

            auto&& instantiatedCallState =
                    CallSiteInitializer(&CI, FN).transform(callState);

            instructionState = (
                FN.State *
                instructionState +
                instantiatedCallState
            ).apply();
        }

        inState = instructionState;
    }

    basicBlockStates[BB] = inState;
}

////////////////////////////////////////////////////////////////////////////////

PredicateState::Ptr OneForAll::BBM(llvm::BasicBlock* BB) {
    using namespace llvm;
    using borealis::util::containsKey;
    using borealis::util::view;

    TRACE_UP("psa::bbm", valueSummary(BB));

    const auto* idom = (*DT)[BB]->getIDom();
    // Function entry block does not have an idom
    if (not idom) return initialState;

    const auto* idomBB = idom->getBlock();
    // idom is unreachable
    if (not containsKey(basicBlockStates, idomBB)) return nullptr;

    auto&& base = basicBlockStates.at(idomBB);
    std::vector<PredicateState::Ptr> choices;

    for (auto* predBB : view(pred_begin(BB), pred_end(BB))) {
        // predecessor is unreachable
        if (not containsKey(basicBlockStates, predBB)) continue;

        auto&& stateBuilder = FN.State * basicBlockStates.at(predBB);

        // Adding path predicate from predBB
        stateBuilder += TPM({predBB->getTerminator(), BB});

        // Adding PHI predicates from predBB
        for (auto&& it = BB->begin(); isa<PHINode>(it); ++it) {
            const auto* phi = cast<PHINode>(it);
            if (-1 != phi->getBasicBlockIndex(predBB)) {
                stateBuilder += PPM({predBB, phi});
            }
        }

        auto&& inState = stateBuilder.apply();

        auto&& slice = inState->sliceOn(base);
        ASSERT(nullptr != slice, "Could not slice state on its predecessor");

        choices.push_back(slice);
    }

    TRACE_DOWN("psa::bbm", valueSummary(BB));

    if (choices.empty()) {
        // All predecessors are unreachable, and so we are also as such...
        return nullptr;
    } else {
        return (
            FN.State *
            base +
            FN.State->Choice(choices)
        ).apply();
    }
}

PredicateState::Ptr OneForAll::PM(const llvm::Instruction* I) {
    using borealis::util::containsKey;

    auto&& res = FN.State->Basic();

    for (auto* APA : PA) {
        auto&& map = APA->getPredicateMap();
        if (containsKey(map, I)) {
            res = res + map.at(I);
        }
    }

    return res << SLT->getLocFor(I);
}

PredicateState::Ptr OneForAll::PPM(const PhiBranch& key) {
    using borealis::util::containsKey;

    auto&& res = FN.State->Basic();

    for (auto* APA : PA) {
        auto&& map = APA->getPhiPredicateMap();
        if (containsKey(map, key)) {
            res = res + map.at(key);
        }
    }

    return res << SLT->getLocFor(key.second);
}

PredicateState::Ptr OneForAll::TPM(const TerminatorBranch& key) {
    using borealis::util::containsKey;

    auto&& res = FN.State->Basic();

    for (auto* APA : PA) {
        auto&& map = APA->getTerminatorPredicateMap();
        if (containsKey(map, key)) {
            res = res + map.at(key);
        }
    }

    return res << SLT->getLocFor(key.first);
}

////////////////////////////////////////////////////////////////////////////////

char OneForAll::ID;
static RegisterPass<OneForAll>
X("one-for-all", "One-for-all predicate state analysis");

} /* namespace borealis */

#include "Util/unmacros.h"
