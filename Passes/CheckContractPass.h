/*
 * CheckContractPass.h
 *
 *  Created on: Feb 27, 2013
 *      Author: ice-phoenix
 */

#ifndef CHECKCONTRACTPASS_H_
#define CHECKCONTRACTPASS_H_

#include <llvm/Pass.h>

#include "Passes/DefectManager.h"
#include "Passes/PredicateStateAnalysis.h"
#include "Passes/MetaInfoTrackerPass.h"
#include "Passes/ProxyFunctionPass.h"
#include "Passes/SlotTrackerPass.h"
#include "Predicate/PredicateFactory.h"
#include "State/PredicateStateFactory.h"
#include "Term/TermFactory.h"

namespace borealis {

class CheckContractPass :
        public borealis::ProxyFunctionPass,
        public borealis::logging::ClassLevelLogging<CheckContractPass>,
        public ShouldBeModularized {

    friend class CallInstVisitor;

public:

    static char ID;

#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("check-contracts")
#include "Util/unmacros.h"

    CheckContractPass();
    CheckContractPass(llvm::Pass*);
    virtual bool runOnFunction(llvm::Function& F);
    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const;
    virtual ~CheckContractPass();

private:

    PredicateStateAnalysis* PSA;
    MetaInfoTrackerPass* MI;
    FunctionManager* FM;
    DefectManager* DM;

    PredicateFactory::Ptr PF;
    PredicateStateFactory::Ptr PSF;
    TermFactory::Ptr TF;

};

} /* namespace borealis */

#endif /* CHECKCONTRACTPASS_H_ */
