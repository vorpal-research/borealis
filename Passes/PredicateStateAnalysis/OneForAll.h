/*
 * OneForAll.h
 *
 *  Created on: Sep 4, 2012
 *      Author: ice-phoenix
 */

#ifndef PREDICATESTATEANALYSIS_ONEFORALL_H_
#define PREDICATESTATEANALYSIS_ONEFORALL_H_

#include <llvm/Analysis/Dominators.h>
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
#include "Passes/PredicateStateAnalysis.h"
#include "Passes/ProxyFunctionPass.h"
#include "Passes/SlotTrackerPass.h"
#include "Predicate/PredicateFactory.h"
#include "State/PredicateStateFactory.h"
#include "Term/TermFactory.h"
#include "Util/passes.hpp"

namespace borealis {

class OneForAll:
        public AbstractPredicateStateAnalysis,
        public ProxyFunctionPass,
        public borealis::logging::ClassLevelLogging<OneForAll>,
        public ShouldBeModularized {

    typedef AbstractPredicateAnalysis::PhiBranch PhiBranch;
    typedef AbstractPredicateAnalysis::TerminatorBranch TerminatorBranch;

    typedef std::map<const llvm::BasicBlock*, PredicateState::Ptr> BasicBlockStates;

public:

    static char ID;

#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("psa-ofa")
#include "Util/unmacros.h"

    OneForAll();
    OneForAll(llvm::Pass* pass);

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    virtual bool runOnFunction(llvm::Function& F) override;
    virtual void print(llvm::raw_ostream&, const llvm::Module*) const override;

private:

    BasicBlockStates basicBlockStates;

    FunctionManager* FM;
    llvm::DominatorTree* DT;

    std::list<AbstractPredicateAnalysis*> PA;

    PredicateStateFactory::Ptr PSF;
    PredicateFactory::Ptr PF;
    TermFactory::Ptr TF;

    virtual void init() override;

    void processBasicBlock(llvm::BasicBlock* BB);

    PredicateState::Ptr BBM(llvm::BasicBlock* BB);
    PredicateState::Ptr PM(const llvm::Instruction* I);
    PredicateState::Ptr PPM(PhiBranch key);
    PredicateState::Ptr TPM(TerminatorBranch key);

};

} /* namespace borealis */

#endif /* PREDICATESTATEANALYSIS_ONEFORALL_H_ */
