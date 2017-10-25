//
// Created by abdullin on 10/17/17.
//

#include <State/Transformer/FilterContractPredicates.h>
#include "Interpreter/IR/Module.h"
#include "Passes/PredicateStateAnalysis/PredicateStateAnalysis.h"
#include "PredicateStateInterpreter.h"
#include "State/Transformer/Interpreter.h"

#include "Util/macros.h"

namespace borealis {

PredicateStateInterpreter::PredicateStateInterpreter() : llvm::ModulePass(ID) {}

bool PredicateStateInterpreter::runOnModule(llvm::Module& M) {
    auto ST = &GetAnalysis<SlotTrackerPass>().doit(this);
    auto module = absint::Module(&M, ST);

    for (auto&& F : util::viewContainer(M)
                    .filter(LAM(f, not f.isDeclaration()))) {
        auto PSA = &GetAnalysis<PredicateStateAnalysis>::doit(this, F);

        auto ps = FilterContractPredicates(FactoryNest()).transform(PSA->getInstructionState(&F.back().back()));
        auto interpreter = absint::Interpreter(FactoryNest(), module.getDomainFactory());
        if (F.getName() == "borealis.globals") continue;
        errs() << F.getName().str() << endl << ps << endl;
        interpreter.transform(ps);
        errs() << "State: " << interpreter.getState() << endl;
    }
    return false;
}

void PredicateStateInterpreter::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();

    AUX<SlotTrackerPass>::addRequired(AU);
    AUX<PredicateStateAnalysis>::addRequiredTransitive(AU);
}

char PredicateStateInterpreter::ID;
static RegisterPass<PredicateStateInterpreter>
X("ps-interpreter", "Pass that performs abstract interpretation on a predicate state");

}   // namespace borealis

#include "Util/unmacros.h"