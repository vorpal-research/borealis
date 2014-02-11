/*
 * OneForOne.cpp
 *
 *  Created on: Feb 14, 2013
 *      Author: ice-phoenix
 */

#include "Logging/tracer.hpp"
#include "Passes/PredicateAnalysis/Defines.def"
#include "Passes/PredicateStateAnalysis/OneForOne.h"
#include "Passes/Tracker/SlotTrackerPass.h"
#include "State/PredicateStateBuilder.h"
#include "State/Transformer/AggregateTransformer.h"
#include "State/Transformer/CallSiteInitializer.h"
#include "Util/util.h"

namespace borealis {

OneForOne::OneForOne() : ProxyFunctionPass(ID) {}
OneForOne::OneForOne(llvm::Pass* pass) : ProxyFunctionPass(ID, pass) {}

void OneForOne::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();

    AUX<FunctionManager>::addRequiredTransitive(AU);
    AUX<SourceLocationTracker>::addRequiredTransitive(AU);
    AUX<SlotTrackerPass>::addRequiredTransitive(AU);

#define HANDLE_ANALYSIS(CLASS) \
    AUX<CLASS>::addRequiredTransitive(AU);
#include "Passes/PredicateAnalysis/Defines.def"
}

bool OneForOne::runOnFunction(llvm::Function& F) {
    init();

    FM = &GetAnalysis< FunctionManager >::doit(this, F);
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
    for (auto& arg : F.getArgumentList()) {
        initialState = initialState << SLT->getLocFor(&arg);
    }

    // Save initial state
    this->initialState = initialState;

    // Queue up entry basic block
    enqueue(nullptr, &F.getEntryBlock(), initialState);

    // Process basic blocks
    processQueue();

    // Finalize results (merge alternatives as PSChoice)
    finalize();

    return false;
}

////////////////////////////////////////////////////////////////////////////////

void OneForOne::init() {
    AbstractPredicateStateAnalysis::init();

    predicateStates.clear();
    PA.clear();

    WorkQueue q;
    std::swap(workQueue, q);
}

void OneForOne::enqueue(
        const llvm::BasicBlock* from,
        const llvm::BasicBlock* to,
        PredicateState::Ptr state) {
    TRACE_UP("psa::queue");
    workQueue.push(std::make_tuple(from, to, state));
}

void OneForOne::processQueue() {
    while (!workQueue.empty()) {
        processBasicBlock(workQueue.front());
        workQueue.pop();
        TRACE_DOWN("psa::queue");
    }
}

void OneForOne::finalize() {
    for (const auto& e : predicateStates) {
        instructionStates[e.first] = FN.State->Choice(e.second);
    }
}

void OneForOne::processBasicBlock(const WorkQueueEntry& wqe) {
    using namespace llvm;
    using borealis::util::containsKey;
    using borealis::util::toString;
    using borealis::util::view;

    const BasicBlock* from = std::get<0>(wqe);
    const BasicBlock* bb = std::get<1>(wqe);
    PredicateState::Ptr inState = std::get<2>(wqe);

    auto fMemId = FM->getMemoryStart(bb->getParent());

    if (PredicateStateAnalysis::CheckUnreachable() && inState->isUnreachableIn(fMemId)) return;

    auto iter = bb->begin();

    // Add incoming predicates from PHI nodes
    for ( ; isa<PHINode>(iter); ++iter) {
        const PHINode* phi = cast<PHINode>(iter);
        if (phi->getBasicBlockIndex(from) != -1) {
            inState = (FN.State * inState + PPM({from, phi}))();
        }
    }

    for (auto& I : view(iter, bb->end())) {

        PredicateState::Ptr modifiedInState = (FN.State * inState + PM(&I))();
        predicateStates[&I].push_back(modifiedInState);

        TRACE_MEASUREMENT(
                "psa::states." + toString(&I),
                predicateStates[&I].size(),
                "at",
                valueSummary(I));

        // Add ensures and summary *after* the CallInst has been processed
        if (isa<CallInst>(I)) {
            CallInst& CI = cast<CallInst>(I);

            auto callState = (
                FN.State *
                FM->getBdy(CI, FN) +
                FM->getEns(CI, FN)
            )();
            auto t = CallSiteInitializer(CI, FN);

            PredicateState::Ptr instantiatedCallState = callState->map(
                [&t](Predicate::Ptr p) { return t.transform(p); }
            );

            modifiedInState = (
                FN.State *
                modifiedInState +
                instantiatedCallState <<
                SLT->getLocFor(&I)
            )();
        }

        inState = modifiedInState;
    }

    processTerminator(*bb->getTerminator(), inState);
}

void OneForOne::processTerminator(
        const llvm::TerminatorInst& I,
        PredicateState::Ptr state) {
    using namespace::llvm;

    if (isa<BranchInst>(I))
    { processBranchInst(cast<BranchInst>(I), state); }
    else if (isa<SwitchInst>(I))
    { processSwitchInst(cast<SwitchInst>(I), state); }
}

void OneForOne::processBranchInst(
        const llvm::BranchInst& I,
        PredicateState::Ptr state) {
    using namespace::llvm;

    if (I.isUnconditional()) {
        const BasicBlock* succ = I.getSuccessor(0);
        enqueue(I.getParent(), succ, state);
    } else {
        const BasicBlock* trueSucc = I.getSuccessor(0);
        const BasicBlock* falseSucc = I.getSuccessor(1);

        PredicateState::Ptr trueState = TPM({&I, trueSucc});
        PredicateState::Ptr falseState = TPM({&I, falseSucc});

        enqueue(I.getParent(), trueSucc, (FN.State * state + trueState)());
        enqueue(I.getParent(), falseSucc, (FN.State * state + falseState)());
    }
}

void OneForOne::processSwitchInst(
        const llvm::SwitchInst& I,
        PredicateState::Ptr state) {
    using namespace::llvm;

    for (auto c = I.case_begin(); c != I.case_end(); ++c) {
        const BasicBlock* caseSucc = c.getCaseSuccessor();
        PredicateState::Ptr caseState = TPM({&I, caseSucc});
        enqueue(I.getParent(), caseSucc, (FN.State * state + caseState)());
    }

    const BasicBlock* defaultSucc = I.getDefaultDest();
    PredicateState::Ptr defaultState = TPM({&I, defaultSucc});
    enqueue(I.getParent(), defaultSucc, (FN.State * state + defaultState)());
}

PredicateState::Ptr OneForOne::PM(const llvm::Instruction* I) {
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

PredicateState::Ptr OneForOne::PPM(PhiBranch key) {
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

PredicateState::Ptr OneForOne::TPM(TerminatorBranch key) {
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

char OneForOne::ID;
static RegisterPass<OneForOne>
X("one-for-one", "One-for-one predicate state analysis");

} /* namespace borealis */
