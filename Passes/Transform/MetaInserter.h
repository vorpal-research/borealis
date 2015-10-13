/*
 * MetaInserter.h
 *
 *  Created on: Feb 7, 2013
 *      Author: belyaev
 */

#ifndef METAINSERTER_H_
#define METAINSERTER_H_

#include <llvm/Pass.h>

#include <Util/passes.hpp>

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

    static std::pair<llvm::Value*, llvm::Value*> liftDebugIntrinsic(llvm::Module& M, llvm::Value*);
    static llvm::Value* unliftDebugIntrinsic(llvm::Module& M, llvm::Value*);
    static void liftAllDebugIntrinsics(llvm::Module& M);
    static void unliftAllDebugIntrinsics(llvm::Module& M);
};

} /* namespace borealis */

#endif /* METAINSERTER_H_ */
