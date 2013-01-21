/*
 * PredicateStateAnalysis.cpp
 *
 *  Created on: Sep 4, 2012
 *      Author: ice-phoenix
 */

#include <llvm/BasicBlock.h>
#include <llvm/Constants.h>
#include <llvm/Support/raw_ostream.h>

#include "Passes/PredicateStateAnalysis.h"
#include "Solver/Z3ExprFactory.h"
#include "Solver/Z3Solver.h"
#include "State/CallSiteInitializer.h"

#include "Logging/tracer.hpp"

#include "Util/macros.h"

namespace borealis {

typedef AbstractPredicateAnalysis::PredicateMap PM;
typedef AbstractPredicateAnalysis::TerminatorPredicateMap TPM;
typedef AbstractPredicateAnalysis::PhiPredicateMap PPM;



bool isUnreachable(const PredicateState& ps) {
    using namespace::z3;

    context ctx;
    Z3ExprFactory z3ef(ctx);
    Z3Solver s(z3ef);

    return !s.checkPathPredicates(ps);
}



PredicateStateAnalysis::PredicateStateAnalysis() : llvm::FunctionPass(ID) {}

void PredicateStateAnalysis::getAnalysisUsage(llvm::AnalysisUsage& Info) const{
    using namespace llvm;

    Info.setPreservesAll();
    Info.addRequiredTransitive<FunctionManager>();
    Info.addRequiredTransitive<SlotTrackerPass>();
    Info.addRequiredTransitive<LoopInfo>();
}

bool PredicateStateAnalysis::runOnFunction(llvm::Function& F) {
    using namespace llvm;

    TRACE_FUNC;

    init();

    FM = &getAnalysis<FunctionManager>();
    LI = &getAnalysis<LoopInfo>();

    for (auto* apaId : AbstractPredicateAnalysis::getRegistered()) {
        APA.push_back(&getAnalysisID<AbstractPredicateAnalysis>(apaId, F));
    }

    PF = PredicateFactory::get(getAnalysis<SlotTrackerPass>().getSlotTracker(F));
    TF = TermFactory::get(getAnalysis<SlotTrackerPass>().getSlotTracker(F));

    if (!getAllLoops(&F, LI).empty()) {
        BYE_BYE(bool, "Cannot analyze predicates when loops are present");
    }

    for (auto* pa : APA) {
        PA = pa;
        enqueue(nullptr, &F.getEntryBlock(), PredicateState());
        processQueue();
    }

    infos() << "= Predicate state analysis results =" << endl;
    for (auto& e : predicateStateMap) {
        infos() << *e.first << endl
                << e.second << endl;
    }
    infos() << "= Done =" << endl;

    return false;
}

void PredicateStateAnalysis::enqueue(
        const llvm::BasicBlock* from,
        const llvm::BasicBlock* to,
        PredicateState state) {
    workQueue.push(std::make_tuple(from, to, state));
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
    PredicateState inState = std::get<2>(wqe);

    if (isUnreachable(inState)) return;

    auto iter = bb->begin();

    // Add incoming predicates from PHI nodes
    for ( ; isa<PHINode>(iter); ++iter) {
        inState = inState
            .addPredicate(ppm[{from, cast<PHINode>(iter)}])
            .addVisited(&*iter);
    }

    for (auto& I : view(iter, bb->end())) {

        PredicateState modifiedInState = inState.addVisited(&I);

        if (isa<CallInst>(I)) {
            CallInst& CI = cast<CallInst>(I);

            PredicateState callState = FM->get(
                    CI,
                    PF.get(),
                    TF.get());
            CallSiteInitializer csi(CI, TF.get());

            PredicateState transformedCallState = callState.map(
                [&csi](Predicate::Ptr p) {
                    return csi.transform(p);
                }
            );

            modifiedInState = modifiedInState.addAll(transformedCallState);

        } else {
            if (!containsKey(pm, &I)) continue;
            modifiedInState = modifiedInState.addPredicate(pm[&I]);
        }

        if (!containsKey(predicateStateMap, &I)) {
            predicateStateMap[&I] = PredicateStateVector();
        }
        predicateStateMap[&I] = predicateStateMap[&I].merge(modifiedInState);

        inState = modifiedInState;
    }

    processTerminator(*bb->getTerminator(), inState);
}

void PredicateStateAnalysis::processTerminator(
        const llvm::TerminatorInst& I,
        const PredicateState& state) {
    using namespace::llvm;

    auto s = state.addVisited(&I);

    if (isa<BranchInst>(I))
    { processBranchInst(cast<BranchInst>(I), s); }
}

void PredicateStateAnalysis::processBranchInst(
        const llvm::BranchInst& I,
        const PredicateState& state) {
    using namespace::llvm;

    TPM& tpm = PA->getTerminatorPredicateMap();

    if (I.isUnconditional()) {
        const BasicBlock* succ = I.getSuccessor(0);
        enqueue(I.getParent(), succ, state);
    } else {
        const BasicBlock* trueSucc = I.getSuccessor(0);
        const BasicBlock* falseSucc = I.getSuccessor(1);

        Predicate::Ptr truePred = tpm[{&I, trueSucc}];
        Predicate::Ptr falsePred = tpm[{&I, falseSucc}];

        enqueue(I.getParent(), trueSucc, state.addPredicate(truePred));
        enqueue(I.getParent(), falseSucc, state.addPredicate(falsePred));
    }
}

char PredicateStateAnalysis::ID;
static llvm::RegisterPass<PredicateStateAnalysis>
X("predicate-state-analysis", "Predicate state analysis");

} /* namespace borealis */

#include "Util/unmacros.h"
