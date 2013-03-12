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

class DefectSummaryPass: public llvm::ModulePass {
public:
    static char ID;

    DefectSummaryPass(): llvm::ModulePass(ID){};
    virtual ~DefectSummaryPass(){};

    virtual void getAnalysisUsage(llvm::AnalysisUsage& Info) const;
    virtual bool runOnModule(llvm::Module&);
    virtual void print(llvm::raw_ostream& O, const llvm::Module*) const;
};



} /* namespace borealis */
#endif /* DEFECTSUMMARYPASS_H_ */
