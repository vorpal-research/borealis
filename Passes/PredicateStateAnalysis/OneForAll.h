/*
 * OneForAll.h
 *
 *  Created on: Sep 4, 2012
 *      Author: ice-phoenix
 */

#ifndef PREDICATESTATEANALYSIS_ONEFORALL_H_
#define PREDICATESTATEANALYSIS_ONEFORALL_H_

#include <llvm/IR/Dominators.h>
#include <llvm/Pass.h>

#include <list>
#include <unordered_map>

#include "Factory/Nest.h"
#include "Logging/logger.hpp"
#include "Passes/Manager/FunctionManager.h"
#include "Passes/PredicateAnalysis/AbstractPredicateAnalysis.h"
#include "Passes/PredicateStateAnalysis/PredicateStateAnalysis.h"
#include "Passes/Tracker/SourceLocationTracker.h"
#include "Passes/Util/ProxyFunctionPass.h"
#include "State/Transformer/StateOptimizer.h"
#include "State/Transformer/Retyper.h"
#include "Util/graph.h"
#include "Util/passes.hpp"

namespace borealis {

class OneForAll:
        public AbstractPredicateStateAnalysis,
        public ProxyFunctionPass,
        public borealis::logging::ClassLevelLogging<OneForAll>,
        public ShouldBeLazyModularized {

    using PhiBranch = AbstractPredicateAnalysis::PhiBranch;
    using TerminatorBranch = AbstractPredicateAnalysis::TerminatorBranch;

    using BasicBlockStates = std::unordered_map<const llvm::BasicBlock*, PredicateState::Ptr>;
    using PredicateAnalyses = std::list<AbstractPredicateAnalysis*>;

public:

    static char ID;

#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("psa-ofa")
#include "Util/unmacros.h"

    OneForAll();
    OneForAll(llvm::Pass* pass);

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    virtual bool runOnFunction(llvm::Function& F) override;

    virtual PredicateState::Ptr getInstructionState(const llvm::Instruction* I) override;

private:

    BasicBlockStates basicBlockStates;
    PredicateAnalyses PA;

    FactoryNest FN;

    StateOptimizer SO;
    Retyper RR;

    TopologicalSorter::Ordered TO;

    llvm::DominatorTree* DT;
    FunctionManager* FM;
    SourceLocationTracker* SLT;

    virtual void init() override;
    virtual void finalize() override;

    void processBasicBlock(const llvm::BasicBlock* BB);
    void finalizeBasicBlock(const llvm::BasicBlock* BB);

    PredicateState::Ptr BBM(const llvm::BasicBlock* BB);
    PredicateState::Ptr PM(const llvm::Instruction* I);
    PredicateState::Ptr PostPM(const llvm::Instruction* I);
    PredicateState::Ptr PPM(const PhiBranch& key);
    PredicateState::Ptr TPM(const TerminatorBranch& key);

};

} /* namespace borealis */

#endif /* PREDICATESTATEANALYSIS_ONEFORALL_H_ */
