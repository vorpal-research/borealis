/*
 * PredicateStateAnalysis.h
 *
 *  Created on: Sep 4, 2012
 *      Author: ice-phoenix
 */

#ifndef PREDICATESTATEANALYSIS_H_
#define PREDICATESTATEANALYSIS_H_

#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Function.h>
#include <llvm/Instructions.h>
#include <llvm/Pass.h>

#include <algorithm>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <tuple>
#include <vector>

#include "Passes/FunctionManager.h"
#include "Passes/AbstractPredicateAnalysis.h"
#include "Passes/SlotTrackerPass.h"
#include "Predicate/PredicateFactory.h"
#include "State/PredicateState.h"
#include "State/PredicateStateVector.h"
#include "Term/TermFactory.h"

#include "Logging/logger.hpp"

namespace borealis {

class PredicateStateAnalysis:
        public llvm::FunctionPass,
        public borealis::logging::ClassLevelLogging<PredicateStateAnalysis> {

public:

    typedef std::map<const llvm::Instruction*, PredicateStateVector> PredicateStateMap;
    typedef std::pair<const llvm::Instruction*, PredicateStateVector> PredicateStateMapEntry;

    typedef std::tuple<const llvm::BasicBlock*, const llvm::BasicBlock*, PredicateState> WorkQueueEntry;
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
    llvm::LoopInfo* LI;

    std::list<AbstractPredicateAnalysis*> APA;
    AbstractPredicateAnalysis* PA;

    PredicateFactory::Ptr PF;
    TermFactory::Ptr TF;

    void processQueue();
    void enqueue(const llvm::BasicBlock*, const llvm::BasicBlock*, PredicateState);
    void processBasicBlock(const WorkQueueEntry&);
    void processTerminator(const llvm::TerminatorInst&, const PredicateState&);
    void processBranchInst(const llvm::BranchInst&, const PredicateState&);

};

} /* namespace borealis */

#endif /* PREDICATESTATEANALYSIS_H_ */
