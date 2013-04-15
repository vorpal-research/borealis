/*
 * DefectSummaryPass.h
 *
 *  Created on: Mar 12, 2013
 *      Author: belyaev
 */

#ifndef DEFECTSUMMARYPASS_H_
#define DEFECTSUMMARYPASS_H_

#include <llvm/Pass.h>

namespace borealis {

class DefectSummaryPass: public llvm::ModulePass,
    public logging::ClassLevelLogging<DefectSummaryPass> {
public:
    static char ID;

#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("defect-summary")
#include "Util/unmacros.h"


    DefectSummaryPass(): llvm::ModulePass(ID) {};
    virtual ~DefectSummaryPass() {};

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const;
    virtual bool runOnModule(llvm::Module&);
    virtual void print(llvm::raw_ostream& O, const llvm::Module* M) const;
};

} /* namespace borealis */

#endif /* DEFECTSUMMARYPASS_H_ */
