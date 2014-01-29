/*
 * UglyGEPKiller.h
 *
 *  Created on: Jan 28, 2014
 *      Author: belyaev
 */

#ifndef UGLYGEPKILLER_H_
#define UGLYGEPKILLER_H_

#include <llvm/Pass.h>

namespace borealis {

/*
 *
 */
class UglyGEPKiller: public llvm::ModulePass {
public:
    static char ID;

    UglyGEPKiller(): llvm::ModulePass(ID) {}
    virtual ~UglyGEPKiller() {}

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    virtual bool runOnModule(llvm::Module&) override;
    virtual void print(llvm::raw_ostream& O, const llvm::Module* M) const override;
};

} /* namespace borealis */

#endif /* UGLYGEPKILLER_H_ */
