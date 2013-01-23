/*
 * PredicateStateAnalysis.h
 *
 *  Created on: Sep 4, 2012
 *      Author: ice-phoenix
 */

#ifndef PREDICATESTATEANALYSIS_H_
#define PREDICATESTATEANALYSIS_H_

#include <llvm/Analysis/LoopInfo.h>
#include <llvm/BasicBlock.h>
#include <llvm/Constants.h>
#include <llvm/Function.h>
#include <llvm/Instructions.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>

#include <algorithm>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <tuple>
#include <vector>

#include "Passes/AbstractPredicateAnalysis.h"
#include "Passes/FunctionManager.h"
#include "Passes/SlotTrackerPass.h"
#include "Predicate/PredicateFactory.h"
#include "Solver/Z3ExprFactory.h"
#include "Solver/Z3Solver.h"
#include "State/CallSiteInitializer.h"
#include "State/PredicateState.h"
#include "State/PredicateStateVector.h"
#include "Term/TermFactory.h"

#include "Logging/logger.hpp"
#include "Logging/tracer.hpp"

#include "Util/macros.h"

namespace borealis {

template<class PredicateAnalysis>
class PredicateStateAnalysis:
        public llvm::FunctionPass,
        public borealis::logging::ClassLevelLogging<PredicateStateAnalysis<PredicateAnalysis> > {

public:

    typedef AbstractPredicateAnalysis::PredicateMap PM;
    typedef AbstractPredicateAnalysis::TerminatorPredicateMap TPM;
    typedef AbstractPredicateAnalysis::PhiPredicateMap PPM;

    typedef std::map<const llvm::Instruction*, PredicateStateVector> PredicateStateMap;
    typedef std::pair<const llvm::Instruction*, PredicateStateVector> PredicateStateMapEntry;

    typedef std::tuple<const llvm::BasicBlock*, const llvm::BasicBlock*, PredicateState> WorkQueueEntry;
    typedef std::queue<WorkQueueEntry> WorkQueue;

    static char ID;
    static constexpr decltype("psa") loggerDomain() { return "psa"; }

    PredicateStateAnalysis() : llvm::FunctionPass(ID) {}

    void getAnalysisUsage(llvm::AnalysisUsage& Info) const{
        using namespace llvm;

        Info.setPreservesAll();
        Info.addRequiredTransitive<FunctionManager>();
        Info.addRequiredTransitive<LoopInfo>();
        Info.addRequiredTransitive<SlotTrackerPass>();

        Info.addRequiredTransitive<PredicateAnalysis>();
    }

    bool runOnFunction(llvm::Function& F) {
        using namespace llvm;

        TRACE_FUNC;

        init();

        FM = &getAnalysis<FunctionManager>();
        LI = &getAnalysis<LoopInfo>();

        PA = &getAnalysis<PredicateAnalysis>();

        PF = PredicateFactory::get(getAnalysis<SlotTrackerPass>().getSlotTracker(F));
        TF = TermFactory::get(getAnalysis<SlotTrackerPass>().getSlotTracker(F));

        if (!getAllLoops(&F, LI).empty()) {
            BYE_BYE(bool, "Cannot analyze predicates when loops are present");
        }

        enqueue(nullptr, &F.getEntryBlock(), PredicateState());
        processQueue();

        infos() << "= Predicate state analysis results =" << endl;
        for (auto& e : predicateStateMap) {
            infos() << *e.first << endl
                    << e.second << endl;
        }
        infos() << "= Done =" << endl;

        return false;
    }

    virtual ~PredicateStateAnalysis() {};

    PredicateStateMap& getPredicateStateMap() {
        return predicateStateMap;
    }

private:

    void init() {
        predicateStateMap.clear();

        WorkQueue q;
        std::swap(workQueue, q);
    }

    PredicateStateMap predicateStateMap;
    WorkQueue workQueue;

    FunctionManager* FM;
    llvm::LoopInfo* LI;

    PredicateAnalysis* PA;

    PredicateFactory::Ptr PF;
    TermFactory::Ptr TF;

    void enqueue(
            const llvm::BasicBlock* from,
            const llvm::BasicBlock* to,
            PredicateState state) {
        workQueue.push(std::make_tuple(from, to, state));
    }

    void processQueue() {
        while (!workQueue.empty()) {
            processBasicBlock(workQueue.front());
            workQueue.pop();
        }
    }

    void processBasicBlock(const WorkQueueEntry& wqe) {
        using namespace llvm;
        using borealis::util::containsKey;
        using borealis::util::view;

        PM& pm = PA->getPredicateMap();
        PPM& ppm = PA->getPhiPredicateMap();

        const BasicBlock* from = std::get<0>(wqe);
        const BasicBlock* bb = std::get<1>(wqe);
        PredicateState inState = std::get<2>(wqe);

        if (inState.isUnreachable()) return;

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

    void processTerminator(
            const llvm::TerminatorInst& I,
            const PredicateState& state) {
        using namespace::llvm;

        auto s = state.addVisited(&I);

        if (isa<BranchInst>(I))
        { processBranchInst(cast<BranchInst>(I), s); }
    }

    void processBranchInst(
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

};

template<class PredicateAnalysis>
char PredicateStateAnalysis<PredicateAnalysis>::ID;

} /* namespace borealis */

#endif /* PREDICATESTATEANALYSIS_H_ */

#include "Util/unmacros.h"
