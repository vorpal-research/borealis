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

#include "DetectNullPass.h"
#include "Logging/logger.hpp"
#include "PredicateStateAnalysis.h"
#include "SlotTrackerPass.h"
#include "SourceLocationTracker.h"

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
	static constexpr decltype("check-null") loggerDomain() { return "check-null"; }

	CheckNullDereferencePass();
	virtual bool runOnFunction(llvm::Function& F);
	virtual void getAnalysisUsage(llvm::AnalysisUsage& Info) const;
	virtual ~CheckNullDereferencePass();

private:

	llvm::AliasAnalysis* AA;
	DetectNullPass* DNP;
	PredicateStateAnalysis* PSA;
	SlotTracker* slotTracker;
	SourceLocationTracker* sourceLocationTracker;

    DetectNullPass::NullPtrSet* ValueNullSet;
    DetectNullPass::NullPtrSet* DerefNullSet;

};

} /* namespace borealis */

#endif /* CHECKNULLDEREFERENCEPASS_H_ */
