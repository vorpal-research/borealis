/*
 * GenInterpolantsPass.h
 *
 *  Created on: Aug 14, 2013
 *      Author: sam
 */

#ifndef INTERPOL_GENINTERPOLANTSPASS_H_
#define INTERPOL_GENINTERPOLANTSPASS_H_

#include <llvm/Pass.h>

#include "Factory/Nest.h"
#include "Logging/logger.hpp"
#include "Passes/Manager/FunctionManager.h"
#include "Passes/PredicateStateAnalysis/PredicateStateAnalysis.h"
#include "Passes/Tracker/SlotTrackerPass.h"
#include "Passes/Util/ProxyFunctionPass.h"
#include "Util/passes.hpp"

namespace borealis {

class GenInterpolantsPass:
        public ProxyFunctionPass,
        public borealis::logging::ClassLevelLogging<GenInterpolantsPass>,
        public ShouldBeModularized {

    friend class RetInstVisitor;

public:

    static char ID;

#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("gen-interpolants")
#include "Util/unmacros.h"

	GenInterpolantsPass();
    GenInterpolantsPass(llvm::Pass* pass);
	virtual bool runOnFunction(llvm::Function& F) override;
	virtual void getAnalysisUsage(llvm::AnalysisUsage& Info) const override;
	virtual ~GenInterpolantsPass();

private:

	FunctionManager* FM;
	PredicateStateAnalysis* PSA;
	FactoryNest FN;

};

} /* namespace borealis */

#endif /* INTERPOL_GENINTERPOLANTSPASS_H_ */
