/*
 * TassadarCheckerPass.cpp
 *
 *  Created on: Feb 6, 2015
 *      Author: belyaev
 */

#include <Executor/ExecutionEngine.h>
#include <iostream>
#include <fstream>

#include <llvm/Pass.h>
#include <llvm/PassAnalysisSupport.h>
#include <llvm/IR/Module.h>
#include <llvm/Target/TargetLibraryInfo.h>


#include "Passes/Checker/Defines.def"
#include "Config/config.h"
#include "Executor/SmtDrivenArbiter.h"
#include "Util/passes.hpp"
#include "Util/collections.hpp"
#include "Util/functional.hpp"
#include "Logging/tracer.hpp"
#include "Passes/Defect/DefectManager.h"

#include "Util/macros.h"

namespace borealis {
namespace {

using namespace borealis::config;

static llvm::GenericValue symbolicPtr(ExecutionEngine& ee) {
    llvm::GenericValue retVal;
    retVal.PointerVal = ee.getSymbolicPointer();
    return retVal;
}

class TassadarCheckerPass : public llvm::ModulePass {
public:
    static char ID;
    TassadarCheckerPass(): llvm::ModulePass(ID) {};

    void getAnalysisUsage(llvm::AnalysisUsage& AU) const override {
        AUX<llvm::DataLayoutPass>::addRequired(AU);
        AUX<llvm::TargetLibraryInfo>::addRequired(AU);
        AUX<VariableInfoTracker>::addRequired(AU);
        AUX<SlotTrackerPass>::addRequired(AU);

        AUX<DefectManager>::addRequired(AU);

#define HANDLE_CHECKER(Checker) \
        AUX<Checker>::addRequiredTransitive(AU);
#include "Passes/Checker/Defines.def"

        AU.setPreservesAll();
    }

    bool runOnModule(llvm::Module& M) override {
        TRACE_FUNC;


        auto&& DM = getAnalysis<DefectManager>();
        for(auto&& defect: DM.getData()) if(auto&& model = DM.getAdditionalInfo(defect).satModel) {
            llvm::Function* func = DM.getAdditionalInfo(defect).where;
            auto st = getAnalysis<SlotTrackerPass>().getSlotTracker(func);

            auto judicator = std::make_shared<SmtDrivenArbiter>(st, model.getUnsafe());



            ExecutionEngine tassadar{&M,
                &getAnalysis<llvm::DataLayoutPass>().getDataLayout(),
                &getAnalysis<llvm::TargetLibraryInfo>(),
                &getAnalysis<SlotTrackerPass>(),
                &getAnalysis<VariableInfoTracker>(),
                judicator
            };


            auto args =
                util::viewContainer(func->getArgumentList())
                    .map(LAM(arg, arg.getType()->isPointerTy() ? symbolicPtr(tassadar) : judicator->map(&arg) ))
                    .toVector();

            try {
                tassadar.runFunction(func, args);

                std::cerr << "Defect not proven:" << std::endl;
                std::cerr << "    " << defect << std::endl;
            } catch(std::exception& ex) {
                errs() << ex.what() << endl;
            }
        }

        return false;
    }

};

char TassadarCheckerPass::ID = 87;
static RegisterPass<TassadarCheckerPass>
X("tassadar-checker", "Run executor on known defects");

} /* namespace */
} /* namespace borealis */

#include "Util/unmacros.h"
