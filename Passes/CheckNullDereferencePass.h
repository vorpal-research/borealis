/*
 * CheckNullDereferencePass.h
 *
 *  Created on: Aug 24, 2012
 *      Author: ice-phoenix
 */

#ifndef CHECKNULLDEREFERENCEPASS_H_
#define CHECKNULLDEREFERENCEPASS_H_

#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Function.h>
#include <llvm/Instructions.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>

#include "Logging/logger.hpp"
#include "Passes/DefectManager.h"
#include "Passes/DetectNullPass.h"
#include "Passes/PredicateStateAnalysis.h"
#include "Passes/ProxyFunctionPass.h"
#include "Passes/SlotTrackerPass.h"
#include "Predicate/PredicateFactory.h"
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
    static constexpr auto loggerDomain() QUICK_RETURN("null-deref-check")
#include "Util/unmacros.h"

	CheckNullDereferencePass();
    CheckNullDereferencePass(llvm::Pass*);
	virtual bool runOnFunction(llvm::Function& F);
	virtual void getAnalysisUsage(llvm::AnalysisUsage& Info) const;
	virtual ~CheckNullDereferencePass();

private:

	llvm::AliasAnalysis* AA;

	PredicateStateAnalysis* PSA;
    DetectNullPass* DNP;

    DefectManager* defectManager;
	SlotTracker* slotTracker;

	PredicateFactory::Ptr PF;
	TermFactory::Ptr TF;

    DetectNullPass::NullPtrSet* ValueNullSet;
    DetectNullPass::NullPtrSet* DerefNullSet;

};

} /* namespace borealis */

#endif /* CHECKNULLDEREFERENCEPASS_H_ */
