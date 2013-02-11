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
#include "Passes/DefaultPredicateAnalysis.h"
#include "Passes/DefectManager.h"
#include "Passes/DetectNullPass.h"
#include "Passes/PassModularizer.hpp"
#include "Passes/ProxyFunctionPass.hpp"
#include "Passes/SlotTrackerPass.h"

namespace borealis {

class CheckNullDereferencePass:
        public ProxyFunctionPass<CheckNullDereferencePass>,
        public borealis::logging::ClassLevelLogging<CheckNullDereferencePass> {

    friend class DerefInstVisitor;
    friend class ValueInstVisitor;

public:

    static char ID;
	typedef PassModularizer<CheckNullDereferencePass> MX;

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

	DefaultPredicateAnalysis::PSA* PSA;
    DetectNullPass* DNP;

    DefectManager* defectManager;
	SlotTracker* slotTracker;

    DetectNullPass::NullPtrSet* ValueNullSet;
    DetectNullPass::NullPtrSet* DerefNullSet;

};

} /* namespace borealis */

#endif /* CHECKNULLDEREFERENCEPASS_H_ */
