#ifndef ONE_FOR_ALL_TD_H
#define ONE_FOR_ALL_TD_H

#include <llvm/Analysis/PostDominators.h>
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
#include "State/Transformer/MarkEraser.h"
#include "Util/graph.h"
#include "Util/passes.hpp"

namespace borealis {

class OneForAllTD:
        public AbstractPredicateStateAnalysis,
        public ProxyFunctionPass,
        public borealis::logging::ClassLevelLogging<OneForAllTD>,
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

    OneForAllTD();
    OneForAllTD(llvm::Pass* pass);

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    virtual bool runOnFunction(llvm::Function& F) override;

    virtual PredicateState::Ptr getInstructionState(const llvm::Instruction* I) override;

    PredicateState::Ptr getFinalState() const { return finalState; }

private:

    std::vector<Predicate::Ptr> predicatesFor(const llvm::Instruction*);

    std::unordered_map<const llvm::BasicBlock*, PredicateState::Ptr> basics;
    std::unordered_map<const llvm::Instruction*, Predicate::Ptr> backmapping;
    PredicateState::Ptr basicPSFor(const llvm::BasicBlock*);
    const llvm::BasicBlock* getIDom(const llvm::BasicBlock* BB);

    PredicateState::Ptr buildFunctionBodyState(const llvm::Function *);

    std::unordered_map<std::pair<const llvm::BasicBlock*, const llvm::BasicBlock*>, PredicateState::Ptr> betweens;
    PredicateState::Ptr stateBetween(const llvm::BasicBlock*, const llvm::BasicBlock*);

    PredicateState::Ptr finalState = nullptr;

    PredicateAnalyses PA;

    FactoryNest FN;

    StateOptimizer SO;
    Retyper RR;
    MarkEraser ME;

    llvm::PostDominatorTree* DT;
    FunctionManager* FM;
    SourceLocationTracker* SLT;


    virtual void init() override;
    virtual void finalize() override;

};

} /* namespace borealis */

#endif // ONE_FOR_ALL_TD_H
