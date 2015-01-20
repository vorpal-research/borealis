/*
 * DefaultPredicateAnalysis.h
 *
 *  Created on: Aug 31, 2012
 *      Author: ice-phoenix
 */

#ifndef DEFAULTPREDICATEANALYSIS_H_
#define DEFAULTPREDICATEANALYSIS_H_

#include <llvm/Pass.h>
#include <llvm/IR/DataLayout.h>

#include "Factory/Nest.h"
#include "Passes/PredicateAnalysis/AbstractPredicateAnalysis.h"
#include "Passes/Tracker/SourceLocationTracker.h"
#include "Passes/Util/ProxyFunctionPass.h"
#include "Util/passes.hpp"

namespace borealis {

class DefaultPredicateAnalysis:
        public AbstractPredicateAnalysis,
        public ProxyFunctionPass,
        public ShouldBeModularized {

    friend class DPAInstVisitor;

public:

    static char ID;

    DefaultPredicateAnalysis();
    DefaultPredicateAnalysis(llvm::Pass*);
    virtual bool runOnFunction(llvm::Function& F) override;
    virtual void getAnalysisUsage(llvm::AnalysisUsage& Info) const override;
    virtual ~DefaultPredicateAnalysis();

private:

    FactoryNest FN;
    SourceLocationTracker* SLT;
    const llvm::DataLayout* TD;

};

} /* namespace borealis */

#endif /* DEFAULTPREDICATEANALYSIS_H_ */
