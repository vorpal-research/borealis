/*
 * MetaInserter.h
 *
 *  Created on: Feb 7, 2013
 *      Author: belyaev
 */

#ifndef METAINSERTER_H_
#define METAINSERTER_H_

#include <llvm/Pass.h>

namespace borealis {

class MetaInserter : public llvm::ModulePass {
public:

    static char ID;

    MetaInserter();
    virtual ~MetaInserter();

    virtual void getAnalysisUsage(llvm::AnalysisUsage & AU) const override {
        AU.setPreservesCFG();
    }

    virtual bool runOnModule(llvm::Module&) override;

};

} /* namespace borealis */

#endif /* METAINSERTER_H_ */
