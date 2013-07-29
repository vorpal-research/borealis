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
#include "Passes/Defect/DefectManager.h"
#include "Passes/Manager/FunctionManager.h"
#include "Passes/PredicateStateAnalysis/PredicateStateAnalysis.h"
#include "Passes/Tracker/MetaInfoTracker.h"
#include "Passes/Util/ProxyFunctionPass.h"
#include "Util/passes.hpp"

namespace borealis {

class CheckContractPass :
        public ProxyFunctionPass,
        public borealis::logging::ClassLevelLogging<CheckContractPass>,
        public ShouldBeModularized {

    friend class CallInstVisitor;

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

    DefectManager* DM;
    FunctionManager* FM;
    MetaInfoTracker* MI;
    PredicateStateAnalysis* PSA;

    FactoryNest FN;

};

} /* namespace borealis */

#endif /* CHECKCONTRACTPASS_H_ */
