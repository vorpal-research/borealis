/*
 * DefaultPredicateAnalysis.h
 *
 *  Created on: Aug 31, 2012
 *      Author: ice-phoenix
 */

#ifndef DEFAULTPREDICATEANALYSIS_H_
#define DEFAULTPREDICATEANALYSIS_H_

#include <llvm/Target/TargetData.h>

#include "Passes/AbstractPredicateAnalysis.h"
#include "Passes/PredicateStateAnalysis.hpp"
#include "Passes/ProxyFunctionPass.h"
#include "Predicate/PredicateFactory.h"
#include "Term/TermFactory.h"
#include "Util/passes.hpp"

namespace borealis {

class DefaultPredicateAnalysis:
        public ProxyFunctionPass,
        public borealis::AbstractPredicateAnalysis,
        public ShouldBeModularized {

    friend class DPAInstVisitor;

public:

    static char ID;
    typedef PredicateStateAnalysis<DefaultPredicateAnalysis> PSA;

    DefaultPredicateAnalysis();
    DefaultPredicateAnalysis(llvm::Pass*);
    virtual bool runOnFunction(llvm::Function& F);
    virtual void getAnalysisUsage(llvm::AnalysisUsage& Info) const;
    virtual ~DefaultPredicateAnalysis();

private:

    PredicateFactory::Ptr PF;
    TermFactory::Ptr TF;
    llvm::TargetData* TD;

};

} /* namespace borealis */

#endif /* DEFAULTPREDICATEANALYSIS_H_ */
