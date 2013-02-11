/*
 * SCCPass.h
 *
 *  Created on: Feb 7, 2013
 *      Author: ice-phoenix
 */

#ifndef SCCPASS_H_
#define SCCPASS_H_

#include <llvm/Pass.h>

namespace borealis {

class SCCPass : public llvm::ModulePass {

public:

    static char ID;

    SCCPass();
    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const;
    virtual bool runOnModule(llvm::Module&);
    virtual ~SCCPass();
};

} /* namespace borealis */

#endif /* SCCPASS_H_ */
