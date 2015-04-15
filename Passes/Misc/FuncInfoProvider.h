//
// Created by belyaev on 4/15/15.
//

#ifndef AURORA_SANDBOX_FUNCINFOPROVIDER_H
#define AURORA_SANDBOX_FUNCINFOPROVIDER_H

#include <llvm/Pass.h>
#include <llvm/Target/TargetLibraryInfo.h>

#include "Codegen/FuncInfo.h"

namespace borealis {

class FuncInfoProvider: public llvm::ImmutablePass {

    struct Impl;
    std::unique_ptr<Impl> pimpl_;

public:
    static char ID;

    FuncInfoProvider();

    const func_info::FuncInfo& getInfo(llvm::LibFunc::Func f);
    const func_info::FuncInfo& getInfo(llvm::Function* f);
    bool FuncInfoProvider::hasInfo(llvm::LibFunc::Func f);
    bool FuncInfoProvider::hasInfo(llvm::Function* f);

    virtual void initializePass() override;

    ~FuncInfoProvider();
};

} /* namespace borealis */

#endif //AURORA_SANDBOX_FUNCINFOPROVIDER_H
