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

#include <list>
#include <map>
#include <queue>
#include <tuple>

#include "Logging/logger.hpp"
#include "Passes/AbstractPredicateAnalysis.h"
#include "Passes/FunctionManager.h"
#include "Passes/ProxyFunctionPass.h"
#include "Passes/SlotTrackerPass.h"
#include "Predicate/PredicateFactory.h"
#include "State/PredicateStateFactory.h"
#include "State/PredicateStateVector.h"
#include "Term/TermFactory.h"
#include "Util/passes.hpp"

namespace borealis {

class PredicateStateAnalysis:
        public ProxyFunctionPass,
        public borealis::logging::ClassLevelLogging<PredicateStateAnalysis>,
        public ShouldBeModularized {

    typedef std::tuple<const llvm::BasicBlock*, const llvm::BasicBlock*, PredicateState::Ptr> WorkQueueEntry;
    typedef std::queue<WorkQueueEntry> WorkQueue;

    typedef AbstractPredicateAnalysis::PredicateMap PredicateMap;
    typedef AbstractPredicateAnalysis::PhiPredicateMap PhiPredicateMap;
    typedef AbstractPredicateAnalysis::PhiBranch PhiBranch;
    typedef AbstractPredicateAnalysis::TerminatorPredicateMap TerminatorPredicateMap;
    typedef AbstractPredicateAnalysis::TerminatorBranch TerminatorBranch;

public:

    typedef std::map<const llvm::Instruction*, PredicateStateVector> PredicateStateMap;
    typedef PredicateStateMap::value_type PredicateStateMapEntry;

    static char ID;

#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("psa")
#include "Util/unmacros.h"

    PredicateStateAnalysis();
    PredicateStateAnalysis(llvm::Pass* pass);

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const;
    virtual bool runOnFunction(llvm::Function& F);
    virtual void print(llvm::raw_ostream&, const llvm::Module*) const;

    virtual ~PredicateStateAnalysis();

    PredicateStateMap& getPredicateStateMap();

private:

    PredicateStateMap predicateStateMap;
    WorkQueue workQueue;

    FunctionManager* FM;
    llvm::LoopInfo* LI;

    std::list<AbstractPredicateAnalysis*> PA;

    PredicateFactory::Ptr PF;
    PredicateStateFactory::Ptr PSF;
    TermFactory::Ptr TF;

    void init();

    void enqueue(
            const llvm::BasicBlock* from,
            const llvm::BasicBlock* to,
            PredicateState::Ptr state);
    void processQueue();
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

#endif /* PREDICATESTATEANALYSIS_H_ */
