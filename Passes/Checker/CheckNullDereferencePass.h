/*
 * CheckNullDereferencePass.h
 *
 *  Created on: Aug 24, 2012
 *      Author: ice-phoenix
 */

#ifndef CHECKNULLDEREFERENCEPASS_H_
#define CHECKNULLDEREFERENCEPASS_H_

#include <llvm/Pass.h>

#include "Factory/Nest.h"
#include "Logging/logger.hpp"
#include "Passes/Checker/CheckManager.h"
#include "Passes/Defect/DefectManager.h"
#include "Passes/Manager/FunctionManager.h"
#include "Passes/PredicateStateAnalysis/PredicateStateAnalysis.h"
#include "Passes/Tracker/NameTracker.h"
#include "Passes/Tracker/SlotTrackerPass.h"
#include "Passes/Tracker/SourceLocationTracker.h"
#include "Passes/Util/ProxyFunctionPass.h"
#include "Util/passes.hpp"

namespace borealis {

template<class T> class CheckHelper;

class CheckNullDereferencePass:
        public ProxyFunctionPass,
        public borealis::logging::ClassLevelLogging<CheckNullDereferencePass>,
        public ShouldBeModularized {

    friend class CheckNullsVisitor;
    friend class CheckHelper<CheckNullDereferencePass>;

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

    PredicateState::Ptr getInstructionState(llvm::Instruction* I);

    CheckManager* CM;

    PredicateStateAnalysis* PSA;

    llvm::AliasAnalysis* AA;
    DefectManager* DM;
    FunctionManager* FM;
    NameTracker* NT;
    SourceLocationTracker* SLT;
    SlotTrackerPass* ST;

    FactoryNest FN;

};

} /* namespace borealis */

#endif /* CHECKNULLDEREFERENCEPASS_H_ */
