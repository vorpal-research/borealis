/*
 * PredicateStateAnalysis.cpp
 *
 *  Created on: Sep 4, 2012
 *      Author: ice-phoenix
 */

#include "PredicateStateAnalysis.h"

#include <llvm/BasicBlock.h>
#include <llvm/Constants.h>
#include <llvm/Support/raw_ostream.h>

#include "Solver/Z3ExprFactory.h"
#include "Solver/Z3Solver.h"
#include "State/CallSiteInitializer.h"

#include "Logging/tracer.hpp"

namespace borealis {

typedef PredicateAnalysis::PredicateMap PM;
typedef PredicateAnalysis::TerminatorPredicateMap TPM;
typedef PredicateAnalysis::PhiPredicateMap PPM;

PredicateStateAnalysis::PredicateStateAnalysis() : llvm::FunctionPass(ID) {}

void PredicateStateAnalysis::getAnalysisUsage(llvm::AnalysisUsage& Info) const{
    using namespace llvm;

    Info.setPreservesAll();
    Info.addRequiredTransitive<FunctionManager>();
    Info.addRequiredTransitive<PredicateAnalysis>();
    Info.addRequiredTransitive<SlotTrackerPass>();
    Info.addRequiredTransitive<LoopInfo>();
    Info.addRequiredTransitive<ScalarEvolution>();
}

bool PredicateStateAnalysis::runOnFunction(llvm::Function& F) {
    using namespace llvm;

    TRACE_FUNC;

    init();

    FM = &getAnalysis<FunctionManager>();
    PA = &getAnalysis<PredicateAnalysis>();
    LI = &getAnalysis<LoopInfo>();
    SE = &getAnalysis<ScalarEvolution>();

    PF = PredicateFactory::get(getAnalysis<SlotTrackerPass>().getSlotTracker(F));
    TF = TermFactory::get(getAnalysis<SlotTrackerPass>().getSlotTracker(F));

    workQueue.push(std::make_tuple(nullptr, &F.getEntryBlock(), PredicateStateVector(true)));
    processQueue();

    for (auto* L : getAllLoops(&F, LI)) {
        processLoop(L);
    }

    removeUnreachableStates();

    infos() << "= Predicate state analysis results =" << endl;
    for (auto& e : predicateStateMap) {
        infos() << *e.first << endl
                << e.second << endl;
    }
    infos() << "= Done =" << endl;

    return false;
}

void PredicateStateAnalysis::processQueue() {
    while (!workQueue.empty()) {
        processBasicBlock(workQueue.front());
        workQueue.pop();
    }
}

void PredicateStateAnalysis::processBasicBlock(const WorkQueueEntry& wqe) {
    using namespace llvm;
    using borealis::util::containsKey;
    using borealis::util::view;

    PM& pm = PA->getPredicateMap();
    PPM& ppm = PA->getPhiPredicateMap();

    const BasicBlock* from = std::get<0>(wqe);
    const BasicBlock* bb = std::get<1>(wqe);
    PredicateStateVector inStateVec = std::get<2>(wqe);

    bool shouldScheduleTerminator = true;

    auto iter = bb->begin();

    if (from) {
        for ( ; isa<PHINode>(iter); ++iter) {
            inStateVec = inStateVec.addPredicate(
                    ppm[std::make_pair(from, cast<PHINode>(iter))]);
        }
    }

    for (auto& I : view(iter, bb->end())) {

        PredicateStateVector stateVec;
        bool hasState = containsKey(predicateStateMap, &I);
        if (hasState) stateVec = predicateStateMap[&I];

        if (isa<CallInst>(I)) {
            CallInst& CI = cast<CallInst>(I);

            PredicateState callState = FM->get(
                    *CI.getCalledFunction(),
                    PF.get(),
                    TF.get());
            CallSiteInitializer csi(CI, TF.get());

            PredicateState state = callState.map([&csi](Predicate::Ptr p) {
                return csi.transform(p);
            });

            PredicateStateVector modifiedInStateVec = inStateVec;
            for (auto& p : state) {
                modifiedInStateVec = modifiedInStateVec.addPredicate(p);
            }

            PredicateStateVector merged =
                    stateVec.merge(modifiedInStateVec);

            if (stateVec == merged) {
                shouldScheduleTerminator = false;
                break;
            }

            predicateStateMap[&I] = inStateVec = merged;

            continue;
        }

        bool hasPredicate = containsKey(pm, &I);
        if (!hasPredicate) continue;

        PredicateStateVector modifiedInStateVec =
                inStateVec.addPredicate(pm[&I]);
        PredicateStateVector merged =
                stateVec.merge(modifiedInStateVec);

        if (stateVec == merged) {
            shouldScheduleTerminator = false;
            break;
        }

        predicateStateMap[&I] = inStateVec = merged;
    }

    if (shouldScheduleTerminator) {
        processTerminator(*bb->getTerminator(), inStateVec);
    }
}

void PredicateStateAnalysis::processTerminator(
        const llvm::TerminatorInst& I,
        const PredicateStateVector& state) {
    using namespace::llvm;

    if (isa<BranchInst>(I))
    { process(cast<BranchInst>(I), state); }
}

void PredicateStateAnalysis::process(
        const llvm::BranchInst& I,
        const PredicateStateVector& state) {
    using namespace::llvm;

    TPM& tpm = PA->getTerminatorPredicateMap();

    if (I.isUnconditional()) {
        const BasicBlock* succ = I.getSuccessor(0);
        workQueue.push(std::make_tuple(I.getParent(), succ, state));
    } else {
        const BasicBlock* trueSucc = I.getSuccessor(0);
        const BasicBlock* falseSucc = I.getSuccessor(1);

        Predicate::Ptr truePred = tpm[std::make_pair(&I, trueSucc)];
        Predicate::Ptr falsePred = tpm[std::make_pair(&I, falseSucc)];

        workQueue.push(std::make_tuple(I.getParent(), trueSucc, state.addPredicate(truePred)));
        workQueue.push(std::make_tuple(I.getParent(), falseSucc, state.addPredicate(falsePred)));
    }
}

bool isUnreachable(const PredicateState& ps) {
    using namespace::z3;

    context ctx;
    Z3ExprFactory z3ef(ctx);
    Z3Solver s(z3ef);

    return !s.checkPathPredicates(ps);
}

void PredicateStateAnalysis::removeUnreachableStates() {
    for (auto& psme : predicateStateMap){
        auto I = psme.first;
        PredicateStateVector psv = psme.second;
        predicateStateMap[I] = psv.remove_if(isUnreachable);
    }
}

void PredicateStateAnalysis::processLoop(llvm::Loop* L) {
    using namespace llvm;

    unsigned TripCount = 0;
    unsigned TripMultiple = 1;
    BasicBlock *LatchBlock = L->getLoopLatch();
    if (LatchBlock) {
        TripCount = SE->getSmallConstantTripCount(L, LatchBlock);
        TripMultiple = SE->getSmallConstantTripMultiple(L, LatchBlock);
    }
}

char PredicateStateAnalysis::ID;
static llvm::RegisterPass<PredicateStateAnalysis>
X("predicate-state", "Predicate state analysis");

} /* namespace borealis */
