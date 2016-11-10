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
#include "Passes/Checker/CheckManager.h"
#include "Passes/Defect/DefectManager.h"
#include "Passes/Manager/FunctionManager.h"
#include "Passes/PredicateStateAnalysis/PredicateStateAnalysis.h"
#include "Passes/Util/ProxyFunctionPass.h"
#include "Util/passes.hpp"

namespace borealis {

template<class T> class CheckHelper;

class CheckOutOfBoundsPass :
        public ProxyFunctionPass,
        public borealis::logging::ClassLevelLogging<CheckOutOfBoundsPass>,
        public ShouldBeModularized {

    friend class GepInstVisitor;
    friend class CheckHelper<CheckOutOfBoundsPass>;

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

    PredicateState::Ptr getInstructionState(llvm::Instruction* I);

    CheckManager* CM;

    llvm::AliasAnalysis* AA;
    DefectManager* DM;
    FunctionManager* FM;
    PredicateStateAnalysis* PSA;

    FactoryNest FN;
};

} /* namespace borealis */

#endif /* CHECKOUTOFBOUNDPASS_H_ */
