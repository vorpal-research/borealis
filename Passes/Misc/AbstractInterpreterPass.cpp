//
// Created by abdullin on 2/10/17.
//

#include <llvm/Support/GraphWriter.h>

#include "AbstractInterpreterPass.h"
#include "Config/config.h"
#include "Interpreter/Checker/NullDereferenceChecker.h"
#include "Interpreter/Checker/OutOfBoundsChecker.h"
#include "Util/passes.hpp"

namespace borealis {

static config::BoolConfigEntry printCFG("absint", "print-cfg");
static config::BoolConfigEntry enableAnalysis("absint", "ai-analysis");

bool AbstractInterpreterPass::runOnModule(llvm::Module& M) {
    auto&& fip = &GetAnalysis<FuncInfoProvider>().doit(this);
    auto&& st = &GetAnalysis<SlotTrackerPass>().doit(this);
    auto&& dm = &GetAnalysis<DefectManager>().doit(this);
    auto&& cgs = &getAnalysis<CallGraphSlicer>();

    using namespace absint;
    auto interpreter = Interpreter(&M, fip, st, cgs);
    interpreter.run();
    auto& module = interpreter.getModule();

    if (not module.getRootFunctions().empty() && enableAnalysis.get(false)) {
        OutOfBoundsChecker(&module, dm, fip).run();
        NullDereferenceChecker(&module, dm).run();
    }
    return false;
}

void AbstractInterpreterPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();

    AUX<FuncInfoProvider>::addRequired(AU);
    AUX<SlotTrackerPass>::addRequired(AU);
    AUX<DefectManager>::addRequired(AU);
    AUX<CallGraphSlicer>::addRequired(AU);
}

char AbstractInterpreterPass::ID;
static RegisterPass<AbstractInterpreterPass>
X("abs-interpreter", "Pass that performs abstract interpretation on a module");

} // namespace borealis

