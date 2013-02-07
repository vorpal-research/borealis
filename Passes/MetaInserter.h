/*
 * MetaInserter.h
 *
 *  Created on: Feb 7, 2013
 *      Author: belyaev
 */

#ifndef METAINSERTER_H_
#define METAINSERTER_H_

#include <llvm/Pass.h>


#include "Codegen/intrinsics_manager.h"

namespace borealis {

class MetaInserter: public llvm::ModulePass {
public:
    static char ID;

    MetaInserter();
    virtual ~MetaInserter();

    virtual void getAnalysisUsage(llvm::AnalysisUsage & AU) const {
        AU.setPreservesCFG();
    }

    virtual bool runOnModule(llvm::Module&);

};

} /* namespace borealis */
#endif /* METAINSERTER_H_ */
