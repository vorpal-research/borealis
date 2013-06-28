/*
 * CheckNullDereferencePass.h
 *
 *  Created on: Aug 24, 2012
 *      Author: ice-phoenix
 */

#ifndef CHECKNULLDEREFERENCEPASS_H_
#define CHECKNULLDEREFERENCEPASS_H_

#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Pass.h>

#include "Logging/logger.hpp"
#include "Passes/Defect/DefectManager.h"
#include "Passes/Misc/DetectNullPass.h"
#include "Passes/PredicateStateAnalysis/PredicateStateAnalysis.h"
#include "Passes/Tracker/SlotTrackerPass.h"
#include "Passes/Util/ProxyFunctionPass.h"
#include "Predicate/PredicateFactory.h"
#include "State/PredicateStateFactory.h"
#include "Term/TermFactory.h"
#include "Util/passes.hpp"

namespace borealis {

class CheckNullDereferencePass:
        public ProxyFunctionPass,
        public borealis::logging::ClassLevelLogging<CheckNullDereferencePass>,
        public ShouldBeModularized {

    friend class DerefInstVisitor;
    friend class ValueInstVisitor;

public:

    static char ID;

#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("check-null-deref")
#include "Util/unmacros.h"

	CheckNullDereferencePass();
    CheckNullDereferencePass(llvm::Pass* pass);
	virtual bool runOnFunction(llvm::Function& F) override;
	virtual void getAnalysisUsage(llvm::AnalysisUsage& Info) const override;
	virtual ~CheckNullDereferencePass();

private:

	llvm::AliasAnalysis* AA;

	PredicateStateAnalysis* PSA;
    DetectNullPass* DNP;

    DefectManager* DM;
	SlotTracker* ST;

	PredicateFactory::Ptr PF;
	PredicateStateFactory::Ptr PSF;
	TermFactory::Ptr TF;

    DetectNullPass::NullPtrSet* ValueNullSet;
    DetectNullPass::NullPtrSet* DerefNullSet;

};

} /* namespace borealis */

#endif /* CHECKNULLDEREFERENCEPASS_H_ */
