/*
 * MallocMutator.h
 *
 *  Created on: Mar 14, 2013
 *      Author: belyaev
 */

#ifndef MALLOCMUTATOR_H_
#define MALLOCMUTATOR_H_

#include <llvm/Pass.h>

namespace borealis {

class MallocMutator: public llvm::ModulePass {

public:

    static char ID;

    MallocMutator() : llvm::ModulePass(ID) {};
    virtual ~MallocMutator() {}

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const;
    virtual bool runOnModule(llvm::Module&);
    virtual void print(llvm::raw_ostream& O, const llvm::Module* M) const;
};

} /* namespace borealis */

#endif /* MALLOCMUTATOR_H_ */
