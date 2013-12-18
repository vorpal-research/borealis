/*
 * OneForOne.h
 *
 *  Created on: Sep 4, 2012
 *      Author: ice-phoenix
 */

#ifndef PREDICATESTATEANALYSIS_ONEFORONE_H_
#define PREDICATESTATEANALYSIS_ONEFORONE_H_

#include <llvm/Pass.h>

#include <list>
#include <queue>
#include <tuple>
#include <unordered_map>

#include "Factory/Nest.h"
#include "Logging/logger.hpp"
#include "Passes/Manager/FunctionManager.h"
#include "Passes/PredicateAnalysis/AbstractPredicateAnalysis.h"
#include "Passes/PredicateStateAnalysis/PredicateStateAnalysis.h"
#include "Passes/Tracker/SourceLocationTracker.h"
#include "Passes/Util/ProxyFunctionPass.h"
#include "Util/passes.hpp"

namespace borealis {

class OneForOne:
        public AbstractPredicateStateAnalysis,
        public ProxyFunctionPass,
        public borealis::logging::ClassLevelLogging<OneForOne>,
        public ShouldBeLazyModularized {

    typedef std::vector<PredicateState::Ptr> PredicateStateVector;
    typedef std::unordered_map<const llvm::Instruction*, PredicateStateVector> PredicateStates;
    typedef PredicateStates::value_type PredicateStatesEntry;

    typedef std::tuple<const llvm::BasicBlock*, const llvm::BasicBlock*, PredicateState::Ptr> WorkQueueEntry;
    typedef std::queue<WorkQueueEntry> WorkQueue;

    typedef AbstractPredicateAnalysis::PhiBranch PhiBranch;
    typedef AbstractPredicateAnalysis::TerminatorBranch TerminatorBranch;

public:

    static char ID;

#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("psa-ofo")
#include "Util/unmacros.h"

    OneForOne();
    OneForOne(llvm::Pass* pass);

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    virtual bool runOnFunction(llvm::Function& F) override;

private:

    PredicateStates predicateStates;
    WorkQueue workQueue;
    std::list<AbstractPredicateAnalysis*> PA;

    FunctionManager* FM;
    llvm::LoopInfo* LI;
    SourceLocationTracker* SLT;

    FactoryNest FN;

    virtual void init() override;

    void enqueue(
            const llvm::BasicBlock* from,
            const llvm::BasicBlock* to,
            PredicateState::Ptr state);
    void processQueue();

    void finalize();

    void processBasicBlock(const WorkQueueEntry& wqe);
    void processTerminator(
            const llvm::TerminatorInst& I,
            PredicateState::Ptr state);
    void processBranchInst(
            const llvm::BranchInst& I,
            PredicateState::Ptr state);
    void processSwitchInst(
            const llvm::SwitchInst& I,
            PredicateState::Ptr state);

    PredicateState::Ptr PM(const llvm::Instruction* I);
    PredicateState::Ptr PPM(PhiBranch key);
    PredicateState::Ptr TPM(TerminatorBranch key);

};

} /* namespace borealis */

#endif /* PREDICATESTATEANALYSIS_ONEFORONE_H_ */
