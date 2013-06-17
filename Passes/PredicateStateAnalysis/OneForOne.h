/*
 * OneForOne.h
 *
 *  Created on: Sep 4, 2012
 *      Author: ice-phoenix
 */

#ifndef PREDICATESTATEANALYSIS_ONEFORONE_H_
#define PREDICATESTATEANALYSIS_ONEFORONE_H_

#include <llvm/BasicBlock.h>
#include <llvm/Constants.h>
#include <llvm/Function.h>
#include <llvm/Instructions.h>
#include <llvm/Pass.h>

#include <list>
#include <map>
#include <queue>
#include <tuple>

#include "Logging/logger.hpp"
#include "Passes/AbstractPredicateAnalysis.h"
#include "Passes/FunctionManager.h"
#include "Passes/PredicateStateAnalysis.h"
#include "Passes/ProxyFunctionPass.h"
#include "Passes/SlotTrackerPass.h"
#include "Predicate/PredicateFactory.h"
#include "State/PredicateStateFactory.h"
#include "Term/TermFactory.h"
#include "Util/passes.hpp"

namespace borealis {

class OneForOne:
        public AbstractPredicateStateAnalysis,
        public ProxyFunctionPass,
        public borealis::logging::ClassLevelLogging<OneForOne>,
        public ShouldBeModularized {

    typedef std::vector<PredicateState::Ptr> PredicateStateVector;
    typedef std::map<const llvm::Instruction*, PredicateStateVector> PredicateStates;
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
    virtual void print(llvm::raw_ostream&, const llvm::Module*) const override;

private:

    PredicateStates predicateStates;
    WorkQueue workQueue;

    FunctionManager* FM;
    llvm::LoopInfo* LI;

    std::list<AbstractPredicateAnalysis*> PA;

    PredicateFactory::Ptr PF;
    PredicateStateFactory::Ptr PSF;
    TermFactory::Ptr TF;

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
