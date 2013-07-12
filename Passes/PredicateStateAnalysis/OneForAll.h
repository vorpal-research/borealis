/*
 * OneForAll.h
 *
 *  Created on: Sep 4, 2012
 *      Author: ice-phoenix
 */

#ifndef PREDICATESTATEANALYSIS_ONEFORALL_H_
#define PREDICATESTATEANALYSIS_ONEFORALL_H_

#include <llvm/Analysis/Dominators.h>
#include <llvm/Pass.h>

#include <list>
#include <unordered_map>

#include "Logging/logger.hpp"
#include "Passes/Manager/FunctionManager.h"
#include "Passes/PredicateAnalysis/AbstractPredicateAnalysis.h"
#include "Passes/PredicateStateAnalysis/PredicateStateAnalysis.h"
#include "Passes/Util/ProxyFunctionPass.h"
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

    typedef std::unordered_map<const llvm::BasicBlock*, PredicateState::Ptr> BasicBlockStates;

public:

    static char ID;

#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("psa-ofa")
#include "Util/unmacros.h"

    OneForAll();
    OneForAll(llvm::Pass* pass);

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    virtual bool runOnFunction(llvm::Function& F) override;

private:

    BasicBlockStates basicBlockStates;
    std::list<AbstractPredicateAnalysis*> PA;

    FunctionManager* FM;
    llvm::DominatorTree* DT;

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
