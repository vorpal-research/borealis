/*
 * BetterDotPrinter.cpp
 *
 *  Created on: Jan 26, 2015
 *      Author: belyaev
 */

#include <llvm/IR/Function.h>
#include <llvm/IR/CFG.h>
#include <llvm/Support/GraphWriter.h>
#include <llvm/Analysis/CFGPrinter.h>

#include "Util/passes.hpp"
#include "Util/util.h"

namespace borealis {

class BetterDotViewer: public llvm::ModulePass {
public:
    static char ID;

    BetterDotViewer();
    virtual bool runOnModule(llvm::Module& M) override;
    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    virtual ~BetterDotViewer();
};

BetterDotViewer::BetterDotViewer(): llvm::ModulePass(ID){}

BetterDotViewer::~BetterDotViewer(){}

bool BetterDotViewer::runOnModule(llvm::Module& M) {
    for(auto&& F : M) {
        if(F.isDeclaration() || F.isIntrinsic()) continue;

        std::string realFileName = llvm::WriteGraph<const llvm::Function*>(&F, "cfg." + F.getName(), false);
        if (realFileName.empty()) continue;

        llvm::DisplayGraph(realFileName, false, llvm::GraphProgram::DOT);
    }

    return false;

}
void BetterDotViewer::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();
}

char BetterDotViewer::ID;
static RegisterPass<BetterDotViewer>
X("better-view-cfg", "Show function control flow graph in a window");

} /* namespace borealis */
