/*
 * OneForAll.cpp
 *
 *  Created on: Feb 14, 2013
 *      Author: ice-phoenix
 */

#include <llvm/Support/CFG.h>

#include "Logging/tracer.hpp"
#include "Passes/PredicateAnalysis/Defines.def"
#include "Passes/PredicateStateAnalysis/OneForAll.h"
#include "Passes/Tracker/SlotTrackerPass.h"
#include "State/PredicateStateBuilder.h"
#include "State/Transformer/AggregateTransformer.h"
#include "State/Transformer/CallSiteInitializer.h"
#include "Util/graph.h"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

OneForAll::OneForAll() : ProxyFunctionPass(ID) {}
OneForAll::OneForAll(llvm::Pass* pass) : ProxyFunctionPass(ID, pass) {}

void OneForAll::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();

    AUX<llvm::DominatorTree>::addRequired(AU);
    AUX<FunctionManager>::addRequiredTransitive(AU);
    AUX<SlotTrackerPass>::addRequiredTransitive(AU);
    AUX<SourceLocationTracker>::addRequiredTransitive(AU);

#define HANDLE_ANALYSIS(CLASS) \
    AUX<CLASS>::addRequiredTransitive(AU);
#include "Passes/PredicateAnalysis/Defines.def"
}

bool OneForAll::runOnFunction(llvm::Function& F) {
    init();

    FM = &GetAnalysis< FunctionManager >::doit(this, F);
    DT = &GetAnalysis< llvm::DominatorTree >::doit(this, F);
    SLT = &GetAnalysis< SourceLocationTracker >::doit(this, F);

    auto* st = GetAnalysis< SlotTrackerPass >::doit(this, F).getSlotTracker(F);
    FN = FactoryNest(st);

#define HANDLE_ANALYSIS(CLASS) \
    PA.push_back(static_cast<AbstractPredicateAnalysis*>(&GetAnalysis<CLASS>::doit(this, F)));
#include "Passes/PredicateAnalysis/Defines.def"

    // Register globals in our predicate state
    PredicateState::Ptr gState = FN.getGlobalState(F.getParent());

    // Register REQUIRES
    PredicateState::Ptr requires = FM->getReq(&F);

    PredicateState::Ptr initialState = (FN.State * gState + requires)();

    // Register arguments as visited values
    for (const auto& arg : F.getArgumentList()) {
        initialState = initialState << SLT->getLocFor(&arg);
    }

    // Save initial state
    this->initialState = initialState;

    // Process basic blocks in topological order
    TopologicalSorter::Result ordered = TopologicalSorter().doit(F);
    ASSERT(!ordered.empty(),
           "No topological order for: " + F.getName().str());
    ASSERT(ordered.getUnsafe().size() == F.getBasicBlockList().size(),
           "Topological order does not include all basic blocks for: " + F.getName().str());

    dbgs() << "Topological sorting for: " << F.getName() << endl;
    for (auto* BB : ordered.getUnsafe()) {
        dbgs() << valueSummary(BB) << endl;
    }
    dbgs() << "End of topological sorting for: " << F.getName() << endl;

    for (auto* BB : ordered.getUnsafe()) {
        processBasicBlock(BB);
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////

void OneForAll::init() {
    AbstractPredicateStateAnalysis::init();
    basicBlockStates.clear();
    PA.clear();
}

void OneForAll::processBasicBlock(llvm::BasicBlock* BB) {
    using namespace llvm;
    using borealis::util::view;

    auto inState = BBM(BB);

    auto fMemId = FM->getMemoryStart(BB->getParent());

    if (inState == nullptr) return;
    if (PredicateStateAnalysis::CheckUnreachable() && inState->isUnreachableIn(fMemId)) return;

    for (const auto& I : view(BB->begin(), BB->end())) {

        auto instructionState = (FN.State * inState + PM(&I))();
        instructionStates[&I] = instructionState;

        // Add ensures and summary *after* the CallInst has been processed
        if (isa<CallInst>(I)) {
            auto& CI = cast<CallInst>(I);

            auto callState = (
                FN.State *
                FM->getBdy(CI, FN) +
                FM->getEns(CI, FN)
            )();
            auto t = CallSiteInitializer(CI, FN);

            auto instantiatedCallState = callState->map(
                [&t](Predicate::Ptr p) { return t.transform(p); }
            );

            instructionState = (
                FN.State *
                instructionState +
                instantiatedCallState <<
                SLT->getLocFor(&I)
            )();
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
    if (!idom) {
        return initialState;
    }

    const auto* idomBB = idom->getBlock();
    // idom is unreachable
    if (!containsKey(basicBlockStates, idomBB)) {
        return nullptr;
    }

    auto base = basicBlockStates.at(idomBB);
    std::vector<PredicateState::Ptr> choices;

    for (const auto* predBB : view(pred_begin(BB), pred_end(BB))) {
        // predecessor is unreachable
        if (!containsKey(basicBlockStates, predBB)) continue;

        auto stateBuilder = FN.State * basicBlockStates.at(predBB);

        // Adding path predicate from predBB
        stateBuilder += TPM({predBB->getTerminator(), BB});

        // Adding PHI predicates from predBB
        for (auto it = BB->begin(); isa<PHINode>(it); ++it) {
            const PHINode* phi = cast<PHINode>(it);
            if (phi->getBasicBlockIndex(predBB) != -1) {
                stateBuilder += PPM({predBB, phi});
            }
        }

        auto inState = stateBuilder();

        auto slice = inState->sliceOn(base);
        ASSERT(slice != nullptr, "Could not slice state on its predecessor");

        choices.push_back(slice);
    }

    TRACE_DOWN("psa::bbm", valueSummary(BB));

    if (choices.empty())
        // All predecessors are unreachable, and we too are as such...
        return nullptr;
    else
        return (
            FN.State *
            base +
            FN.State->Choice(choices)
        )();
}

PredicateState::Ptr OneForAll::PM(const llvm::Instruction* I) {
    using borealis::util::containsKey;

    PredicateState::Ptr res = FN.State->Basic();

    for (AbstractPredicateAnalysis* APA : PA) {
        auto& map = APA->getPredicateMap();
        if (containsKey(map, I)) {
            res = res + map.at(I);
        }
    }

    return res << SLT->getLocFor(I);
}

PredicateState::Ptr OneForAll::PPM(PhiBranch key) {
    using borealis::util::containsKey;

    PredicateState::Ptr res = FN.State->Basic();

    for (AbstractPredicateAnalysis* APA : PA) {
        auto& map = APA->getPhiPredicateMap();
        if (containsKey(map, key)) {
            res = res + map.at(key);
        }
    }

    return res << SLT->getLocFor(key.second);
}

PredicateState::Ptr OneForAll::TPM(TerminatorBranch key) {
    using borealis::util::containsKey;

    PredicateState::Ptr res = FN.State->Basic();

    for (AbstractPredicateAnalysis* APA : PA) {
        auto& map = APA->getTerminatorPredicateMap();
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
