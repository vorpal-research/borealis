//
// Created by abdullin on 10/17/17.
//

#include "Codegen/intrinsics_manager.h"
#include "Interpreter/IR/Module.h"
#include "Passes/Checker/CallGraphSlicer.h"
#include "Passes/PredicateStateAnalysis/PredicateStateAnalysis.h"
#include "PredicateStateInterpreter.h"
#include "State/Transformer/ContractPredicatesFilterer.h"
#include "State/Transformer/GlobalVariableFinder.h"
#include "State/Transformer/PSInterpreter.h"

#include "Util/macros.h"

namespace borealis {

PredicateStateInterpreter::PredicateStateInterpreter() : ProxyFunctionPass(ID) {}

PredicateStateInterpreter::PredicateStateInterpreter(llvm::Pass* pass) : ProxyFunctionPass(ID, pass) {}

bool PredicateStateInterpreter::runOnFunction(llvm::Function& F) {
    auto&& cgs = GetAnalysis<CallGraphSlicer>::doit(this);
    if(cgs.doSlicing() && !cgs.getSlice().count(&F)) return false;
    IntrinsicsManager& im = IntrinsicsManager::getInstance();
    if (function_type::UNKNOWN != im.getIntrinsicType(&F)) return false;

    auto ST = &GetAnalysis<SlotTrackerPass>().doit(this, F);

    auto PSA = &GetAnalysis<PredicateStateAnalysis>::doit(this, F);
    auto module = absint::Module(F.getParent(), ST);

    auto FN = FactoryNest(F.getParent()->getDataLayout(), ST->getSlotTracker(F));
    auto initState = PSA->getInstructionState(&F.back().back());

    auto globFinder = GlobalVariableFinder(FN);
    auto searched = globFinder.transform(initState);
    auto filtered = ContractPredicatesFilterer(FN).transform(searched);
    module.initGlobals(globFinder.getGlobals());

    auto interpreter = absint::PSInterpreter(FN, module.getDomainFactory());
    interpreter.transform(filtered);
    return false;
}

void PredicateStateInterpreter::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();

    AUX<SlotTrackerPass>::addRequired(AU);
    AUX<PredicateStateAnalysis>::addRequiredTransitive(AU);
    AUX<CallGraphSlicer>::addRequired(AU);
}

char PredicateStateInterpreter::ID;
static RegisterPass<PredicateStateInterpreter>
X("ps-interpreter", "Pass that performs abstract interpretation on a predicate state");

}   // namespace borealis

#include "Util/unmacros.h"