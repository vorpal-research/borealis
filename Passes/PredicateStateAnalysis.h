/*
 * PredicateStateAnalysis.h
 *
 *  Created on: Sep 4, 2012
 *      Author: ice-phoenix
 */

#ifndef PREDICATESTATEANALYSIS_H_
#define PREDICATESTATEANALYSIS_H_

#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Function.h>
#include <llvm/Instructions.h>
#include <llvm/Pass.h>

#include <algorithm>
#include <map>
#include <queue>
#include <set>
#include <tuple>
#include <vector>

#include "FunctionManager.h"
#include "PredicateAnalysis.h"
#include "SlotTrackerPass.h"

#include "Logging/logger.hpp"
#include "Predicate/PredicateFactory.h"
#include "State/PredicateState.h"
#include "State/PredicateStateVector.h"
#include "Term/TermFactory.h"

namespace borealis {

class PredicateStateAnalysis:
        public llvm::FunctionPass,
        public borealis::logging::ClassLevelLogging<PredicateStateAnalysis> {

public:

    typedef std::map<const llvm::Instruction*, PredicateStateVector> PredicateStateMap;
    typedef std::pair<const llvm::Instruction*, PredicateStateVector> PredicateStateMapEntry;

    typedef std::tuple<
            const llvm::BasicBlock*,
            const llvm::BasicBlock*,
            PredicateStateVector>
    WorkQueueEntry;
    typedef std::queue<WorkQueueEntry> WorkQueue;

    static char ID;
    static constexpr decltype("psa") loggerDomain() { return "psa"; }

    PredicateStateAnalysis();
    virtual bool runOnFunction(llvm::Function& F);
    virtual void getAnalysisUsage(llvm::AnalysisUsage& Info) const;
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
    PredicateAnalysis* PA;
    llvm::LoopInfo* LI;
    llvm::ScalarEvolution* SE;

    PredicateFactory::Ptr PF;
    TermFactory::Ptr TF;

    void processQueue();
    void processBasicBlock(const WorkQueueEntry& wqe);
    void processTerminator(const llvm::TerminatorInst& I, const PredicateStateVector& state);
    void process(const llvm::BranchInst& I, const PredicateStateVector& state);

    void processLoop(llvm::Loop* L);

    void removeUnreachableStates();

};

} /* namespace borealis */

#endif /* PREDICATESTATEANALYSIS_H_ */
