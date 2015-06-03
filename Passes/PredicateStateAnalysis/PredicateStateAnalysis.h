/*
 * PredicateStateAnalysis.h
 *
 *  Created on: Sep 4, 2012
 *      Author: ice-phoenix
 */

#ifndef PREDICATESTATEANALYSIS_H_
#define PREDICATESTATEANALYSIS_H_

#include <llvm/IR/Dominators.h>
#include <llvm/Pass.h>

#include <unordered_map>

#include "Factory/Nest.h"
#include "Logging/logger.hpp"
#include "Passes/Manager/FunctionManager.h"
#include "Passes/Location/LocationManager.h"
#include "Passes/PredicateAnalysis/AbstractPredicateAnalysis.h"
#include "Passes/Util/ProxyFunctionPass.h"
#include "State/PredicateState.h"
#include "Util/passes.hpp"
#include "Util/util.h"

namespace borealis {

////////////////////////////////////////////////////////////////////////////////

class AbstractPredicateStateAnalysis {

public:

    using InstructionStates = std::unordered_map<const llvm::Instruction*, PredicateState::Ptr>;

    AbstractPredicateStateAnalysis();
    virtual ~AbstractPredicateStateAnalysis() = default;

    virtual bool runOnFunction(llvm::Function& F) = 0;

    void printInstructionStates(llvm::raw_ostream&, const llvm::Module*) const;

    PredicateState::Ptr getInitialState() const;
    PredicateState::Ptr getInstructionState(const llvm::Instruction* I) const;

protected:

    PredicateState::Ptr initialState;
    InstructionStates instructionStates;

    virtual void init();
    virtual void finalize();

};

////////////////////////////////////////////////////////////////////////////////

class PredicateStateAnalysis:
        public ProxyFunctionPass,
        public borealis::logging::ClassLevelLogging<PredicateStateAnalysis>,
        public ShouldBeLazyModularized {

    using InstructionStates = AbstractPredicateStateAnalysis::InstructionStates;

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
    static bool OptimizeStates();

private:

    AbstractPredicateStateAnalysis* delegate;

    FactoryNest FN;

    void updateInlineSummary(llvm::Function& F);
    void updateInterpolSummary(llvm::Function& F);
    void updateVisitedLocs(llvm::Function&);

};

} /* namespace borealis */

#endif /* PREDICATESTATEANALYSIS_H_ */
