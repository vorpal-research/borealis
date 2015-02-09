/*
 * TassadarPass.cpp
 *
 *  Created on: Feb 6, 2015
 *      Author: belyaev
 */

#include <llvm/Pass.h>
#include <llvm/PassAnalysisSupport.h>
#include <llvm/IR/Module.h>

#include "Config/config.h"
#include "Executor/Executor.h"

#include "Util/passes.hpp"
#include "Util/collections.hpp"

#include "Util/macros.h"

#include "Logging/tracer.hpp"

namespace borealis {
namespace {

using namespace borealis::config;
MultiConfigEntry functionsToRun {"executor", "function"};

class TassadarPass : public llvm::ModulePass {
public:
    static char ID;
    TassadarPass(): llvm::ModulePass(ID) {};

    void getAnalysisUsage(llvm::AnalysisUsage& AU) const override {
        AUX<llvm::DataLayoutPass>::addRequired(AU);

        AU.setPreservesAll();
    }

    bool runOnModule(llvm::Module& M) override {
        TRACE_FUNC;

        auto funcNames = util::viewContainer(functionsToRun).toHashSet();
        Executor tassadar{&M, &getAnalysis<llvm::DataLayoutPass>().getDataLayout()};

        auto funcs = util::viewContainer(M)
                    .filter(LAM(F, funcNames.count(F.getName())))
                    .map(LAM(F, &F))
                    .toHashSet();

        for(auto&& F : funcs) tassadar.runFunction(F, {});

        return false;
    }

};

char TassadarPass::ID = 87;
static RegisterPass<TassadarPass>
X("tassadar", "Run executor");

} /* namespace */
} /* namespace borealis */

#include "Util/unmacros.h"
