/*
 * CheckContractPass.h
 *
 *  Created on: Feb 27, 2013
 *      Author: ice-phoenix
 */

#ifndef CHECKCONTRACTPASS_H_
#define CHECKCONTRACTPASS_H_

#include <llvm/Pass.h>

#include "Factory/Nest.h"
#include "Logging/logger.hpp"
#include "Passes/Checker/CheckManager.h"
#include "Passes/Defect/DefectManager.h"
#include "Passes/Manager/FunctionManager.h"
#include "Passes/PredicateStateAnalysis/PredicateStateAnalysis.h"
#include "Passes/Tracker/VariableInfoTracker.h"
#include "Passes/Util/ProxyFunctionPass.h"
#include "Util/passes.hpp"

namespace borealis {

template<class T> class CheckHelper;

class CheckContractPass :
        public ProxyFunctionPass,
        public borealis::logging::ClassLevelLogging<CheckContractPass>,
        public ShouldBeModularized {

    friend class CallInstVisitor;
    friend class CheckHelper<CheckContractPass>;

public:

    static char ID;

#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("check-contracts")
#include "Util/unmacros.h"

    CheckContractPass();
    CheckContractPass(llvm::Pass* pass);
    virtual bool runOnFunction(llvm::Function& F) override;
    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    virtual ~CheckContractPass();

private:

    PredicateState::Ptr getInstructionState(llvm::Instruction* I);

    CheckManager* CM;

    llvm::AliasAnalysis* AA;
    DefectManager* DM;
    FunctionManager* FM;
    VariableInfoTracker* MI;
    PredicateStateAnalysis* PSA;

    FactoryNest FN;

};

} /* namespace borealis */

#endif /* CHECKCONTRACTPASS_H_ */
