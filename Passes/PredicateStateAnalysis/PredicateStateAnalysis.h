/*
 * PredicateStateAnalysis.h
 *
 *  Created on: Sep 4, 2012
 *      Author: ice-phoenix
 */

#ifndef PREDICATESTATEANALYSIS_H_
#define PREDICATESTATEANALYSIS_H_

#include <llvm/Analysis/Dominators.h>
#include <llvm/Pass.h>

#include <unordered_map>

#include "Logging/logger.hpp"
#include "Passes/Manager/FunctionManager.h"
#include "Passes/PredicateAnalysis/AbstractPredicateAnalysis.h"
#include "Passes/Util/ProxyFunctionPass.h"
#include "State/PredicateState.h"
#include "Util/passes.hpp"
#include "Util/util.h"

namespace borealis {

////////////////////////////////////////////////////////////////////////////////

class AbstractPredicateStateAnalysis {

public:

    typedef std::unordered_map<const llvm::Instruction*, PredicateState::Ptr> InstructionStates;

    AbstractPredicateStateAnalysis();
    virtual ~AbstractPredicateStateAnalysis();

    void printInstructionStates(llvm::raw_ostream&, const llvm::Module*) const {
        infos() << "Predicate state analysis results" << endl;
        infos() << "Initial state" << endl
                << initialState << endl;
        for (const auto& e : instructionStates) {
            infos() << *e.first << endl
                    << e.second << endl;
        }
        infos() << "End of predicate state analysis results" << endl;
    }

    PredicateState::Ptr getInitialState() const {
        return initialState;
    }

    PredicateState::Ptr getInstructionState(const llvm::Instruction* I) const {
        if (borealis::util::containsKey(instructionStates, I)) {
            return instructionStates.at(I);
        } else {
            return nullptr;
        }
    }

protected:

    PredicateState::Ptr initialState;
    InstructionStates instructionStates;

    virtual void init() {
        initialState.reset();
        instructionStates.clear();
    }

};

////////////////////////////////////////////////////////////////////////////////

class PredicateStateAnalysis:
        public ProxyFunctionPass,
        public borealis::logging::ClassLevelLogging<PredicateStateAnalysis>,
        public ShouldBeModularized {

    typedef AbstractPredicateStateAnalysis::InstructionStates InstructionStates;

public:

    static char ID;

#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("psa")
#include "Util/unmacros.h"

    PredicateStateAnalysis();
    PredicateStateAnalysis(llvm::Pass* pass);

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    virtual bool runOnFunction(llvm::Function& F) override;
    virtual void print(llvm::raw_ostream& O , const llvm::Module* M) const override;

    PredicateState::Ptr getInstructionState(const llvm::Instruction* I) const;

    static const std::string Mode();
    static bool CheckUnreachable();
    static const std::string Summaries();

private:

    AbstractPredicateStateAnalysis* delegate;

};

} /* namespace borealis */

#endif /* PREDICATESTATEANALYSIS_H_ */
