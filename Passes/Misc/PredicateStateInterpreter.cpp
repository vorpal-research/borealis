//
// Created by abdullin on 10/17/17.
//

#include "Passes/PredicateStateAnalysis/PredicateStateAnalysis.h"
#include "PredicateStateInterpreter.h"

namespace borealis {

PredicateStateInterpreter::PredicateStateInterpreter() : ProxyFunctionPass(ID) {}
PredicateStateInterpreter::PredicateStateInterpreter(llvm::Pass* pass) : ProxyFunctionPass(ID, pass) {}

bool PredicateStateInterpreter::runOnFunction(llvm::Function& F) {
    auto PSA = &GetAnalysis<PredicateStateAnalysis>::doit(this, F);

    auto ps = PSA->getInstructionState(&F.back().back());
    errs() << ps << endl;
    return false;
}

void PredicateStateInterpreter::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();

    AUX<PredicateStateAnalysis>::addRequiredTransitive(AU);
}

char PredicateStateInterpreter::ID;
static RegisterPass<PredicateStateInterpreter>
X("ps-interpreter", "Pass that performs abstract interpretation on a predicate state");

}   // namespace borealis