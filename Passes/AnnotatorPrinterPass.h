/*
 * AnnotatorPrinterPass.h
 *
 *  Created on: Jan 10, 2013
 *      Author: belyaev
 */

#ifndef ANNOTATORPRINTERPASS_H_
#define ANNOTATORPRINTERPASS_H_

#include <llvm/Pass.h>
#include "Logging/logger.hpp"

namespace borealis {

class AnnotatorPrinterPass:
        public llvm::ModulePass,
        public borealis::logging::ClassLevelLogging<AnnotatorPrinterPass>{
public:
    static char ID;
#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("annotator")
#include "Util/unmacros.h"

    AnnotatorPrinterPass();
    virtual ~AnnotatorPrinterPass();

    virtual void getAnalysisUsage(llvm::AnalysisUsage& Info) const;
    virtual bool runOnModule(llvm::Module&);
};

} /* namespace borealis */
#endif /* ANNOTATORPRINTERPASS_H_ */
