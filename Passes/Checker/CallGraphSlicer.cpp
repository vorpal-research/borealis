//
// Created by belyaev on 12/17/15.
//

#include "Passes/Checker/CallGraphSlicer.h"
#include "Util/cast.hpp"
#include "Util/passes.hpp"
#include "Config/config.h"

#include <unordered_set>
#include <queue>

#include "Util/macros.h"

namespace borealis {

static config::MultiConfigEntry roots{"analysis", "root-function"};

bool CallGraphSlicer::runOnModule(llvm::Module& M) {

    if(roots.size() == 0) return false;

    std::unordered_set<const llvm::Function*> keep;
    for(auto&& name: roots) {
        if(auto&& F = M.getFunction(name)) keep.insert(F);
    }

    std::queue<const llvm::Function*> que;
    for(auto&& F : keep) que.push(F);

    while(!que.empty()) {
        auto F = que.front();
        que.pop();

        if(F->isDeclaration()) continue;
        if(keep.count(F)) continue;

        auto called =
            util::viewContainer(*F)
            .flatten()
            .filter(LAM(it, llvm::is_one_of<llvm::CallInst, llvm::InvokeInst>(it)))
            .map(LAM(i, llvm::ImmutableCallSite(&i).getCalledFunction()))
            .filter();

        for(auto&& F: called) {
            keep.insert(F);
            que.push(F);
        }
    }

    slice = std::move(keep);

    return false;
}

void CallGraphSlicer::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    llvm::Pass::getAnalysisUsage(AU);
    AU.setPreservesAll();
}

CallGraphSlicer::CallGraphSlicer(): llvm::ImmutablePass(ID) {}
CallGraphSlicer::~CallGraphSlicer() {}

char CallGraphSlicer::ID;
static RegisterPass<CallGraphSlicer> X("cfg-slicer", "Pass that masks out unused functions");

} /* namespace borealis */

#include "Util/unmacros.h"
