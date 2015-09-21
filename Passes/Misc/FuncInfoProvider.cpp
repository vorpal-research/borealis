//
// Created by belyaev on 4/15/15.
//

#include <fstream>

#include "Annotation/AnnotationCast.h"
#include "Util/passes.hpp"
#include "Passes/Tracker/SlotTrackerPass.h"
#include "Passes/Tracker/SourceLocationTracker.h"
#include "Passes/Misc/FuncInfoProvider.h"
#include "Config/config.h"
#include "Factory/Nest.h"

#include "Util/macros.h"

namespace borealis {

struct FuncInfoProvider::Impl {
    llvm::TargetLibraryInfo* TLI;

    std::unordered_map<llvm::LibFunc::Func, func_info::FuncInfo> functions;
    std::unordered_map<llvm::LibFunc::Func, std::vector<Annotation::Ptr>> contracts;
};

char FuncInfoProvider::ID = 42;
static RegisterPass<FuncInfoProvider> X("func-info", "Provide function descriptions from external file");

static config::MultiConfigEntry FunctionDefinitionFiles("analysis", "ext-functions");

FuncInfoProvider::FuncInfoProvider() : llvm::ModulePass(ID), pimpl_(std::make_unique<Impl>()) {}
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
    return !!pimpl_->functions.count(f);
}

bool FuncInfoProvider::hasInfo(llvm::Function* f) {
    llvm::LibFunc::Func enumf;
    if(!pimpl_->TLI->getLibFunc(f->getName(), enumf)) return false;
    return hasInfo(enumf);
}

void FuncInfoProvider::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();
    AUX<llvm::TargetLibraryInfo>::addRequiredTransitive(AU);
    AUX<borealis::SlotTrackerPass>::addRequiredTransitive(AU);
}

bool FuncInfoProvider::runOnModule(llvm::Module& M) {
    pimpl_->TLI = &getAnalysis<llvm::TargetLibraryInfo>();
    auto&& ST = getAnalysis<SlotTrackerPass>();
    auto FN = FactoryNest(ST.getSlotTracker(M));


    for(auto&& filename : FunctionDefinitionFiles) {
        std::ifstream input(util::getFilePathIfExists(filename));
        Json::Value allShit;
        input >> allShit;
        for(auto&& val : allShit){
            auto opt = util::fromJson< func_info::FuncInfo >(val);
            if(!opt) {
                errs() << "cannot parse json: " << val;
            } else {
                auto lfn = util::fromString<llvm::LibFunc::Func>(opt->id).getOrElse(llvm::LibFunc::NumLibFuncs);
                if(lfn == llvm::LibFunc::NumLibFuncs) {
                    errs() << "function " << opt->id << " not resolved" << endl;
                    continue;
                }

                auto func = M.getFunction(pimpl_->TLI->getName(lfn));

                if(func) {
                    if(func->isVarArg()) opt->argInfo.resize(func->arg_size() + 1);
                    else opt->argInfo.resize(func->arg_size());

                    pimpl_->functions[lfn] = *opt;
                    pimpl_->contracts[lfn] = util::viewContainer(opt->contracts)
                        .map(LAM(A, borealis::fromString(Locus{}, A, FN.Term)))
                        .filter()
                        .toVector();
                }
            }
        }
    }
    return false;
}

const std::vector<Annotation::Ptr> FuncInfoProvider::getContracts(llvm::LibFunc::Func f) {
    return pimpl_->contracts.at(f);
}

const std::vector<Annotation::Ptr> FuncInfoProvider::getContracts(llvm::Function* f) {
    llvm::LibFunc::Func enumf;
    pimpl_->TLI->getLibFunc(f->getName(), enumf);
    return getContracts(enumf);
}
} /* namespace borealis */

#include "Util/unmacros.h"
