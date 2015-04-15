//
// Created by belyaev on 4/15/15.
//

#include <fstream>

#include "Util/passes.hpp"
#include "Passes/Misc/FuncInfoProvider.h"

#include "Util/generate_macros.h"

namespace borealis {

struct FuncInfoProvider::Impl {
    llvm::TargetLibraryInfo* TLI;

    std::unordered_map<llvm::LibFunc::Func, func_info::FuncInfo> functions;
};

char FuncInfoProvider::ID = 42;
static RegisterPass<FuncInfoProvider> X("func-info", "Provide external function descriptions from external file");

FuncInfoProvider::FuncInfoProvider() : ImmutablePass(ID), pimpl_(std::make_unique<Impl>()) {}
FuncInfoProvider::~FuncInfoProvider() {}

const func_info::FuncInfo& FuncInfoProvider::getInfo(llvm::LibFunc::Func f) {
    return pimpl_->functions.at(f);
}

const func_info::FuncInfo& FuncInfoProvider::getInfo(llvm::Function* f) {
    llvm::LibFunc::Func enumf;
    pimpl_->TLI->getLibFunc(f->getName(), enumf);
    return pimpl_->functions.at(enumf);
}

bool FuncInfoProvider::hasInfo(llvm::LibFunc::Func f) {
    return pimpl_->functions.count(f);
}

bool FuncInfoProvider::hasInfo(llvm::Function* f) {
    llvm::LibFunc::Func enumf;
    pimpl_->TLI->getLibFunc(f->getName(), enumf);
    return hasInfo(enumf);
}

void FuncInfoProvider::initializePass() {
    llvm::ImmutablePass::initializePass();

    pimpl_->TLI = &getAnalysis<llvm::TargetLibraryInfo>();

    std::ifstream input("stdLib.json");
    std::vector<func_info::FuncInfo> ffs;
    input >> util::jsonify(ffs);

    pimpl_->functions = util::viewContainer(ffs).map([&](auto&& x){
        return std::make_pair(util::fromString<llvm::LibFunc::Func>(x.id).getOrElse(llvm::LibFunc::NumLibFuncs),
                              std::move(x));
    }).to<std::unordered_map<llvm::LibFunc::Func, func_info::FuncInfo>>();
}

} /* namespace borealis */


