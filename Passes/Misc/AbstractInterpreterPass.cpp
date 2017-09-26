//
// Created by abdullin on 2/10/17.
//

#include <llvm/Support/GraphWriter.h>

#include "AbstractInterpreterPass.h"
#include "Config/config.h"
#include "Interpreter/Checker/NullDereferenceChecker.h"
#include "Interpreter/Checker/OutOfBoundsChecker.h"
#include "Interpreter/IR/GraphTraits.hpp"
#include "Util/passes.hpp"

namespace borealis {

static config::BoolConfigEntry printCFG("absint", "print-cfg");
static config::BoolConfigEntry enableAnalysis("absint", "ai-analysis");

bool AbstractInterpreterPass::runOnModule(llvm::Module& M) {
    auto&& fip = &GetAnalysis<FuncInfoProvider>().doit(this);
    auto&& st = &GetAnalysis<SlotTrackerPass>().doit(this);
    auto&& dm = &GetAnalysis<DefectManager>().doit(this);

    auto interpreter = absint::Interpreter(&M, fip, st);
    interpreter.run();

    if (M.getFunction("main") && enableAnalysis.get(false)) {
        auto* module = const_cast<absint::Module*>(&interpreter.getModule());
        absint::OutOfBoundsChecker(module, dm, fip).run();
        absint::NullDereferenceChecker(module, dm).run();
    }
    return false;
}

void AbstractInterpreterPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();

    AUX<FuncInfoProvider>::addRequired(AU);
    AUX<SlotTrackerPass>::addRequired(AU);
    AUX<DefectManager>::addRequired(AU);
}

char AbstractInterpreterPass::ID;
static RegisterPass<AbstractInterpreterPass>
X("abs-interpreter", "Pass that performs abstract interpretation on a module");

} // namespace borealis

