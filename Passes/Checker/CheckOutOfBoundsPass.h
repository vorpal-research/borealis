/*
 * CheckOutOfBoundPass.h
 *
 *  Created on: Sep 2, 2013
 *      Author: sam
 */

#ifndef CHECKOUTOFBOUNDPASS_H_
#define CHECKOUTOFBOUNDPASS_H_

#include <llvm/Pass.h>

#include "Factory/Nest.h"
#include "Logging/logger.hpp"
#include "Passes/Defect/DefectManager.h"
#include "Passes/PredicateStateAnalysis/PredicateStateAnalysis.h"
#include "Passes/Util/ProxyFunctionPass.h"
#include "Util/passes.hpp"

namespace borealis {

class CheckOutOfBoundsPass :
        public ProxyFunctionPass,
        public borealis::logging::ClassLevelLogging<CheckOutOfBoundsPass>,
        public ShouldBeModularized {

    friend class GepInstVisitor;

public:

    static char ID;

#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("check-bounds")
#include "Util/unmacros.h"

    CheckOutOfBoundsPass();
    CheckOutOfBoundsPass(llvm::Pass* pass);
    virtual bool runOnFunction(llvm::Function& F) override;
    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    virtual ~CheckOutOfBoundsPass();

private:

    DefectManager* DM;
    PredicateStateAnalysis* PSA;

    FactoryNest FN;
};

} /* namespace borealis */

#endif /* CHECKOUTOFBOUNDPASS_H_ */
