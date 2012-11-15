/*
 * InitializationPass.h
 *
 *  Created on: Nov 15, 2012
 *      Author: ice-phoenix
 */

#ifndef INITIALIZATIONPASS_H_
#define INITIALIZATIONPASS_H_

#include <llvm/Pass.h>

namespace borealis {

class InitializationPass: public llvm::ImmutablePass {

public:

    static char ID;

    InitializationPass(): llvm::ImmutablePass(ID) {}

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const;
    virtual void initializePass();

    virtual ~InitializationPass();

};

} /* namespace borealis */

#endif /* INITIALIZATIONPASS_H_ */
