//
// Created by belyaev on 4/15/15.
//

#ifndef AURORA_SANDBOX_FUNCINFOPROVIDER_H
#define AURORA_SANDBOX_FUNCINFOPROVIDER_H

#include <llvm/Pass.h>
#include <llvm/Target/TargetLibraryInfo.h>

#include "Codegen/FuncInfo.h"

namespace borealis {

class FuncInfoProvider: public llvm::ModulePass {

    struct Impl;
    std::unique_ptr<Impl> pimpl_;

public:
    static char ID;

    FuncInfoProvider();

    void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;

    const func_info::FuncInfo& getInfo(const llvm::Function* f);
    const std::vector<Annotation::Ptr>& getContracts(llvm::Function* f);
    bool hasInfo(llvm::Function* f);

    virtual bool runOnModule(llvm::Module &M) override;

    ~FuncInfoProvider();
};

} /* namespace borealis */

#endif //AURORA_SANDBOX_FUNCINFOPROVIDER_H
