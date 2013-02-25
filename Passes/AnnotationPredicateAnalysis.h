/*
 * AnnotationPredicateAnalysis.h
 *
 *  Created on: Feb 25, 2013
 *      Author: ice-phoenix
 */

#ifndef ANNOTATIONPREDICATEANALYSIS_H_
#define ANNOTATIONPREDICATEANALYSIS_H_

#include <llvm/Pass.h>

#include "Passes/AbstractPredicateAnalysis.h"
#include "Passes/ProxyFunctionPass.h"
#include "Predicate/PredicateFactory.h"
#include "Term/TermFactory.h"
#include "Util/passes.hpp"

namespace borealis {

class AnnotationPredicateAnalysis:
        public ProxyFunctionPass,
        public AbstractPredicateAnalysis,
        public ShouldBeModularized {

    friend class APAInstVisitor;

public:

    static char ID;

    AnnotationPredicateAnalysis();
    AnnotationPredicateAnalysis(llvm::Pass*);
    virtual bool runOnFunction(llvm::Function& F);
    virtual void getAnalysisUsage(llvm::AnalysisUsage& Info) const;
    virtual ~AnnotationPredicateAnalysis();

private:

    PredicateFactory::Ptr PF;
    TermFactory::Ptr TF;

};

} /* namespace borealis */

#endif /* ANNOTATIONPREDICATEANALYSIS_H_ */
