/*
 * PredicateStateAnalysis.h
 *
 *  Created on: Sep 4, 2012
 *      Author: ice-phoenix
 */

#ifndef PREDICATESTATEANALYSIS_H_
#define PREDICATESTATEANALYSIS_H_

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

#include "Logging/logger.hpp"
#include "Logging/tracer.hpp"
#include "Passes/AbstractPredicateAnalysis.h"
#include "Passes/FunctionManager.h"
#include "Passes/ProxyFunctionPass.hpp"
#include "Passes/SlotTrackerPass.h"
#include "Predicate/PredicateFactory.h"
#include "Solver/Z3ExprFactory.h"
#include "Solver/Z3Solver.h"
#include "State/CallSiteInitializer.h"
#include "State/PredicateState.h"
#include "State/PredicateStateVector.h"
#include "Term/TermFactory.h"
#include "Util/passes.hpp"

#include "Util/macros.h"

namespace borealis {

template<class PredicateAnalysis>
class PredicateStateAnalysis:
        public ProxyFunctionPass< PredicateStateAnalysis<PredicateAnalysis> >,
        public borealis::logging::ClassLevelLogging<PredicateStateAnalysis<PredicateAnalysis> >,
        public ShouldBeModularized {

public:

    typedef PredicateStateAnalysis<PredicateAnalysis> self;

    typedef AbstractPredicateAnalysis::PredicateMap PM;
    typedef AbstractPredicateAnalysis::TerminatorPredicateMap TPM;
    typedef AbstractPredicateAnalysis::PhiPredicateMap PPM;

    typedef std::map<const llvm::Instruction*, PredicateStateVector> PredicateStateMap;
    typedef PredicateStateMap::value_type PredicateStateMapEntry;

    typedef std::tuple<const llvm::BasicBlock*, const llvm::BasicBlock*, PredicateState> WorkQueueEntry;
    typedef std::queue<WorkQueueEntry> WorkQueue;

    static char ID;

    static constexpr auto loggerDomain() QUICK_RETURN("psa")

    PredicateStateAnalysis() : ProxyFunctionPass<self>() {}
    PredicateStateAnalysis(llvm::Pass* pass) : ProxyFunctionPass<self>(pass) {}

    void getAnalysisUsage(llvm::AnalysisUsage& AU) const{
        using namespace llvm;

        AU.setPreservesAll();

        AUX<FunctionManager>::addRequiredTransitive(AU);
        AUX<SlotTrackerPass>::addRequiredTransitive(AU);

        AUX<PredicateAnalysis>::addRequiredTransitive(AU);
    }

    bool runOnFunction(llvm::Function& F) {
        using namespace llvm;

        TRACE_FUNC;

        init();

        FM = & (GetAnalysis< FunctionManager >::doit(this, F));

        auto* ST = GetAnalysis< SlotTrackerPass >::doit(this, F).getSlotTracker(F);
        PF = PredicateFactory::get(ST);
        TF = TermFactory::get(ST);

        PA = & (GetAnalysis< PredicateAnalysis >::doit(this, F));

        enqueue(nullptr, &F.getEntryBlock(), PredicateState());
        processQueue();

        return false;
    }

    virtual void print(llvm::raw_ostream&, const llvm::Module*) const {
        infos() << "= Predicate state analysis results =" << endl;
        for (auto& e : predicateStateMap) {
            infos() << *e.first << endl
                    << e.second << endl;
        }
        infos() << "= Done =" << endl;
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
            const PHINode* phi = cast<PHINode>(iter);
            if (phi->getBasicBlockIndex(from) != -1) {
                inState = inState
                    .addPredicate(ppm[{from, phi}])
                    .addVisited(&*iter);
            }
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
        // FIXME: Add support for SwitchInst
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
