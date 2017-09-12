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
    std::queue<const llvm::Function*> que;

    for(auto&& name: roots) {
        if(auto&& F = M.getFunction(name)) que.push(F);
    }

    for(auto&& F : M) {
        if(llvm::hasAddressTaken(F)) que.push(&F);
    }

    while(!que.empty()) {
        auto F = que.front();
        que.pop();

        if(F->isDeclaration()) continue;
        if(keep.count(F)) continue;
        keep.insert(F);

        auto called =
            util::viewContainer(*F)
            .flatten()
            .filter(LAM(it, llvm::is_one_of<llvm::CallInst, llvm::InvokeInst>(it)))
            .map(LAM(i, llvm::ImmutableCallSite(&i).getCalledFunction()))
            .filter()
            .toHashSet();

        for(auto&& F: called) {
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

CallGraphSlicer::CallGraphSlicer(): llvm::ModulePass(ID) {}
CallGraphSlicer::~CallGraphSlicer() {}

char CallGraphSlicer::ID;
static RegisterPass<CallGraphSlicer> X("callgraph-slicer", "Pass that masks out unused functions");

} /* namespace borealis */

#include "Util/unmacros.h"
