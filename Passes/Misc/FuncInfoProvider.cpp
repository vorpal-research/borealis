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

#include <tinyformat/tinyformat.h>

#include "Util/macros.h"

namespace borealis {

struct FuncInfoProvider::Impl {
    llvm::TargetLibraryInfo* TLI;

    std::unordered_map<std::string, func_info::FuncInfo> functions;
    std::unordered_map<std::string, std::vector<Annotation::Ptr>> contracts;

    llvm::StringRef getNameIgnoringState(llvm::LibFunc::Func f) {
        if(TLI->has(f)) {
            return TLI->getName(f);
        } else {
            TLI->setAvailable(f);
            auto name = TLI->getName(f);
            TLI->setUnavailable(f);
            return name;
        }
    }
};

char FuncInfoProvider::ID = 42;
static RegisterPass<FuncInfoProvider> X("func-info", "Provide function descriptions from external file");

static config::MultiConfigEntry FunctionDefinitionFiles("analysis", "ext-functions");

FuncInfoProvider::FuncInfoProvider() : llvm::ModulePass(ID), pimpl_(std::make_unique<Impl>()) {}
FuncInfoProvider::~FuncInfoProvider() {}

const func_info::FuncInfo& FuncInfoProvider::getInfo(llvm::Function* f) {
    return pimpl_->functions.at(f->getName().str());
}

bool FuncInfoProvider::hasInfo(llvm::Function* f) {
    return !!pimpl_->functions.count(f->getName().str());
}

void FuncInfoProvider::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();
    AUX<llvm::TargetLibraryInfo>::addRequiredTransitive(AU);
    AUX<borealis::SlotTrackerPass>::addRequiredTransitive(AU);
}


bool FuncInfoProvider::runOnModule(llvm::Module& M) {
    pimpl_->TLI = &getAnalysis<llvm::TargetLibraryInfo>();
    auto&& ST = getAnalysis<SlotTrackerPass>();
    auto FN = FactoryNest(M.getDataLayout(), ST.getSlotTracker(M));


    for(auto&& filename : FunctionDefinitionFiles) {
        std::ifstream input(util::getFilePathIfExists(filename));
        Json::Value allShit;
        input >> allShit;
        for(auto&& val : allShit){
            auto opt = util::fromJson< func_info::FuncInfo >(val);
            if(!opt) {
                errs() << "cannot parse json: " << val;
            } else {
                std::string fname = opt->id;

                auto lfn = util::fromString<llvm::LibFunc::Func>(opt->id).getOrElse(llvm::LibFunc::NumLibFuncs);

                if(lfn != llvm::LibFunc::NumLibFuncs) {
                    // XXX: this is somewhat fucked up
                    fname = pimpl_->getNameIgnoringState(lfn);
                }

                auto func = M.getFunction(fname);

                if(func) {
                    if(func->isVarArg()) opt->argInfo.resize(func->arg_size() + 1);
                    else opt->argInfo.resize(func->arg_size());

                    pimpl_->functions[fname] = *opt;
                    pimpl_->contracts[fname] = util::viewContainer(opt->contracts)
                        .map(LAM(A, borealis::fromString(Locus{}, A, FN.Term)))
                        .filter()
                        .toVector();
                }
            }
        }
    }
    return false;
}

const std::vector<Annotation::Ptr>& FuncInfoProvider::getContracts(llvm::Function* f) {
    auto key = f->getName().str();
    return pimpl_->contracts.at(key);
}
} /* namespace borealis */

#include "Util/unmacros.h"
