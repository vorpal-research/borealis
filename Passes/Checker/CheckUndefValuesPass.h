/*
 * CheckUndefValuesPass.h
 *
 *  Created on: Apr 18, 2013
 *      Author: ice-phoenix
 */

#ifndef CHECKUNDEFVALUESPASS_H_
#define CHECKUNDEFVALUESPASS_H_

#include <llvm/Pass.h>

#include "Logging/logger.hpp"
#include "Passes/DefectManager.h"
#include "Passes/ProxyFunctionPass.h"
#include "Util/passes.hpp"

namespace borealis {

class CheckUndefValuesPass :
        public ProxyFunctionPass,
        public borealis::logging::ClassLevelLogging<CheckUndefValuesPass>,
        public ShouldBeModularized {

    friend class UndefInstVisitor;

public:

    static char ID;

#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("check-undefs")
#include "Util/unmacros.h"

    CheckUndefValuesPass();
    CheckUndefValuesPass(llvm::Pass* pass);
    virtual bool runOnFunction(llvm::Function& F) override;
    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    virtual ~CheckUndefValuesPass();

private:

    DefectManager* DM;

};

} /* namespace borealis */

#endif /* CHECKUNDEFVALUESPASS_H_ */
