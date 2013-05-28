/*
 * PredicateStateAnalysis.h
 *
 *  Created on: Sep 4, 2012
 *      Author: ice-phoenix
 */

#ifndef PREDICATESTATEANALYSIS_H_
#define PREDICATESTATEANALYSIS_H_

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
#include "Passes/ProxyFunctionPass.h"
#include "Passes/SlotTrackerPass.h"
#include "Predicate/PredicateFactory.h"
#include "State/PredicateStateFactory.h"
#include "Term/TermFactory.h"
#include "Util/passes.hpp"

namespace borealis {

class PredicateStateAnalysis:
        public ProxyFunctionPass,
        public borealis::logging::ClassLevelLogging<PredicateStateAnalysis>,
        public ShouldBeModularized {

    typedef AbstractPredicateAnalysis::PredicateMap PredicateMap;
    typedef AbstractPredicateAnalysis::PhiPredicateMap PhiPredicateMap;
    typedef AbstractPredicateAnalysis::PhiBranch PhiBranch;
    typedef AbstractPredicateAnalysis::TerminatorPredicateMap TerminatorPredicateMap;
    typedef AbstractPredicateAnalysis::TerminatorBranch TerminatorBranch;

public:

    typedef std::map<const llvm::BasicBlock*, PredicateState::Ptr> BasicBlockStates;
    typedef std::map<const llvm::Instruction*, PredicateState::Ptr> InstructionStates;

    static char ID;

#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("psa")
#include "Util/unmacros.h"

    PredicateStateAnalysis();
    PredicateStateAnalysis(llvm::Pass* pass);

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const;
    virtual bool runOnFunction(llvm::Function& F);
    virtual void print(llvm::raw_ostream&, const llvm::Module*) const;

    const InstructionStates& getInstructionStates();

private:

    BasicBlockStates basicBlockStates;
    InstructionStates instructionStates;

    FunctionManager* FM;
    llvm::DominatorTree* DT;

    std::list<AbstractPredicateAnalysis*> PA;

    PredicateFactory::Ptr PF;
    PredicateStateFactory::Ptr PSF;
    TermFactory::Ptr TF;

    void init();

    void processBasicBlock(llvm::BasicBlock* BB);

    PredicateState::Ptr BBM(llvm::BasicBlock* BB);
    PredicateState::Ptr PM(const llvm::Instruction* I);
    PredicateState::Ptr PPM(PhiBranch key);
    PredicateState::Ptr TPM(TerminatorBranch key);

};

} /* namespace borealis */

#endif /* PREDICATESTATEANALYSIS_H_ */
