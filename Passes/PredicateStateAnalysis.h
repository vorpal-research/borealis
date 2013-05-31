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
#include "Util/util.h"

namespace borealis {

////////////////////////////////////////////////////////////////////////////////

class AbstractPredicateStateAnalysis {

public:

    typedef std::map<const llvm::Instruction*, PredicateState::Ptr> InstructionStates;

    AbstractPredicateStateAnalysis();
    virtual ~AbstractPredicateStateAnalysis();

    virtual bool runOnFunction(llvm::Function& F) = 0;
    virtual void print(llvm::raw_ostream&, const llvm::Module*) const = 0;

    PredicateState::Ptr getInstructionState(const llvm::Instruction* I) const {
        if (borealis::util::containsKey(instructionStates, I)) {
            return instructionStates.at(I);
        } else {
            return nullptr;
        }
    }

protected:

    InstructionStates instructionStates;

    virtual void init() {
        instructionStates.clear();
    }

};

////////////////////////////////////////////////////////////////////////////////

class PredicateStateAnalysis:
        public ProxyFunctionPass,
        public ShouldBeModularized {

    typedef AbstractPredicateStateAnalysis::InstructionStates InstructionStates;

public:

    static char ID;

    PredicateStateAnalysis();
    PredicateStateAnalysis(llvm::Pass* pass);

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    virtual bool runOnFunction(llvm::Function& F) override;
    virtual void print(llvm::raw_ostream& O , const llvm::Module* M) const override;

    PredicateState::Ptr getInstructionState(const llvm::Instruction* I) const;

    static const std::string Mode;

private:

    AbstractPredicateStateAnalysis* delegate;

};

} /* namespace borealis */

#endif /* PREDICATESTATEANALYSIS_H_ */
