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

#include "Passes/DefaultPredicateAnalysis.h"
#include "Passes/DefectManager.h"
#include "Passes/DetectNullPass.h"
#include "Passes/SlotTrackerPass.h"

#include "Logging/logger.hpp"

namespace borealis {

class DerefInstVisitor;
class ValueInstVisitor;

class CheckNullDereferencePass:
        public llvm::FunctionPass,
        public borealis::logging::ClassLevelLogging<CheckNullDereferencePass> {

    friend class DerefInstVisitor;
    friend class ValueInstVisitor;

public:

	static char ID;
#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("null-deref-check")
#include "Util/unmacros.h"

	CheckNullDereferencePass();
	virtual bool runOnFunction(llvm::Function& F);
	virtual void getAnalysisUsage(llvm::AnalysisUsage& Info) const;
	virtual ~CheckNullDereferencePass();

private:

	llvm::AliasAnalysis* AA;
	DefaultPredicateAnalysis::PSA* PSA;
    DefectManager* defectManager;
    DetectNullPass* DNP;
	SlotTracker* slotTracker;

    DetectNullPass::NullPtrSet* ValueNullSet;
    DetectNullPass::NullPtrSet* DerefNullSet;

};

} /* namespace borealis */

#endif /* CHECKNULLDEREFERENCEPASS_H_ */
