//
// Created by abdullin on 2/10/17.
//

#include <llvm/Support/GraphWriter.h>

#include "IRInterpreterPass.h"
#include "Config/config.h"
#include "Interpreter/Checker/ContractChecker.h"
#include "Interpreter/Checker/NullDereferenceChecker.h"
#include "Interpreter/Checker/OutOfBoundsChecker.h"
#include "Util/passes.hpp"

namespace borealis {

static config::BoolConfigEntry enableAnalysis("absint", "enable-ir-interpreter");

bool IRInterpreterPass::runOnModule(llvm::Module& M) {
    if (not enableAnalysis.get(false)) return false;

    auto&& fip = &GetAnalysis<FuncInfoProvider>().doit(this);
    auto&& st = &GetAnalysis<SlotTrackerPass>().doit(this);
    auto&& dm = &GetAnalysis<DefectManager>().doit(this);
    auto&& cgs = &getAnalysis<CallGraphSlicer>();

    using namespace absint;
    auto interpreter = ir::Interpreter(&M, fip, st, cgs);
    interpreter.run();
    auto& module = interpreter.getModule();

    if (not module.getRootFunctions().empty()) {
        ir::OutOfBoundsChecker(&module, dm, fip).run();
        ir::NullDereferenceChecker(&module, dm).run();
    }
    return false;
}

void IRInterpreterPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();

    AUX<FuncInfoProvider>::addRequired(AU);
    AUX<SlotTrackerPass>::addRequired(AU);
    AUX<DefectManager>::addRequired(AU);
    AUX<CallGraphSlicer>::addRequired(AU);
}

char IRInterpreterPass::ID;
static RegisterPass<IRInterpreterPass>
X("ir-interpreter", "Pass that performs abstract interpretation on a LLVM IR Module");

} // namespace borealis

