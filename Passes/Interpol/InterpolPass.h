/*
 * InterpolPass.h
 *
 *  Created on: Jul 15, 2013
 *      Author: Sam Kolton
 */

#ifndef INTERPOLPASS_H_
#define INTERPOLPASS_H_

#include <llvm/Pass.h>

#include "Logging/logger.hpp"
#include "Passes/Defect/DefectManager.h"
#include "Passes/Manager/FunctionManager.h"
#include "Passes/Misc/DetectNullPass.h"
#include "Passes/PredicateStateAnalysis/PredicateStateAnalysis.h"
#include "Passes/Tracker/MetaInfoTracker.h"
#include "Passes/Util/ProxyFunctionPass.h"
#include "Predicate/PredicateFactory.h"
#include "State/PredicateStateFactory.h"
#include "Term/TermFactory.h"
#include "Util/passes.hpp"

namespace borealis {

class InterpolPass :
        public ProxyFunctionPass,
        public borealis::logging::ClassLevelLogging<InterpolPass>,
        public ShouldBeModularized {

    friend class RetInstVisitor;

public:

    static char ID;

#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("gen-interpol")
#include "Util/unmacros.h"

	InterpolPass();
    InterpolPass(llvm::Pass* pass);
    virtual bool runOnFunction(llvm::Function& F) override;
    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    virtual ~InterpolPass();

private:

    DefectManager* DM;
    PredicateStateAnalysis* PSA;
    DetectNullPass* DNP;

    PredicateFactory::Ptr PF;
    PredicateStateFactory::Ptr PSF;
    TermFactory::Ptr TF;

};

} /* namespace borealis */

#endif /* INTERPOLPASS_H_ */
