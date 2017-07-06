//
// Created by abdullin on 2/10/17.
//

#include <llvm/Support/GraphWriter.h>

#include "AbstractInterpreterPass.h"
#include "Config/config.h"
#include "FuncInfoProvider.h"
#include "Interpreter/Checker/OutOfBoundsChecker.h"
#include "Interpreter/IR/GraphTraits.hpp"
#include "Util/passes.hpp"

namespace borealis {

static config::BoolConfigEntry printCFG("absint", "print-cfg");

bool AbstractInterpreterPass::runOnModule(llvm::Module& M) {
    auto&& fip = &GetAnalysis<FuncInfoProvider>().doit(this);
    auto&& st = &GetAnalysis<SlotTrackerPass>().doit(this);
    auto&& dm = &GetAnalysis<DefectManager>().doit(this);

    auto interpreter = absint::Interpreter(&M, fip, st);
    interpreter.run();

    if (printCFG.get(false)) {
        viewAbsintCFG(interpreter.getModule());
    }

    if (M.getFunction("main")) {
        absint::OutOfBoundsChecker(const_cast<absint::Module*>(&interpreter.getModule()), dm).run();
    }
    return false;
}

void AbstractInterpreterPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();

    AUX<FuncInfoProvider>::addRequired(AU);
    AUX<SlotTrackerPass>::addRequired(AU);
    AUX<DefectManager>::addRequired(AU);
}

void AbstractInterpreterPass::viewAbsintCFG(const absint::Module& module) {
    for (auto&& function : module.getFunctions()) {
        std::string realFileName = llvm::WriteGraph<absint::Function*>(function.second.get(),
                                                                       "absint." + function.second->getName(),
                                                                       false);
        if (realFileName.empty()) continue;

        llvm::DisplayGraph(realFileName, false, llvm::GraphProgram::DOT);
    }
}

char AbstractInterpreterPass::ID;
static RegisterPass<AbstractInterpreterPass>
X("abs-interpreter", "Pass that performs abstract interpretation on a module");

} // namespace borealis

