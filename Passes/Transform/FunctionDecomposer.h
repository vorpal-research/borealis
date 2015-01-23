/*
 * FunctionDecomposer.h
 *
 *  Created on: Jan 23, 2015
 *      Author: belyaev
 */

#ifndef PASSES_TRANSFORM_FUNCTIONDECOMPOSER_H_
#define PASSES_TRANSFORM_FUNCTIONDECOMPOSER_H_

#include <llvm/Pass.h>

namespace borealis {

class FunctionDecomposer : public llvm::ModulePass {
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
public:
    static char ID;

    FunctionDecomposer();
    virtual ~FunctionDecomposer();

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    virtual bool runOnModule(llvm::Module&) override;
    virtual void print(llvm::raw_ostream& O, const llvm::Module* M) const override;
};

} /* namespace borealis */

#endif /* PASSES_TRANSFORM_FUNCTIONDECOMPOSER_H_ */
