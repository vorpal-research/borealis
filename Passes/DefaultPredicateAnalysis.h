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
#include "Predicate/PredicateFactory.h"
#include "Term/TermFactory.h"

#include "Passes/PredicateStateAnalysis.hpp"

namespace borealis {

class DefaultPredicateAnalysis:
        public llvm::FunctionPass,
        public borealis::AbstractPredicateAnalysis {

public:

    typedef PredicateStateAnalysis<DefaultPredicateAnalysis> PSA;

    static char ID;

    DefaultPredicateAnalysis();
    virtual bool runOnFunction(llvm::Function& F);
    virtual void getAnalysisUsage(llvm::AnalysisUsage& Info) const;

private:

    PredicateFactory::Ptr PF;
    TermFactory::Ptr TF;
    llvm::TargetData* TD;

    friend class DPAInstVisitor;

};

} /* namespace borealis */

#endif /* DEFAULTPREDICATEANALYSIS_H_ */
