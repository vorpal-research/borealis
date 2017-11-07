//
// Created by abdullin on 10/27/17.
//

#include <State/Transformer/OutOfBoundsChecker.h>
#include "Interpreter/IR/Module.h"
#include "State/Transformer/GlobalVariableFinder.h"
#include "State/Transformer/ContractPredicatesFilterer.h"
#include "State/Transformer/Interpreter.h"
#include "PSInterpreter.h"

namespace borealis {
namespace absint {

PSInterpreter::PSInterpreter(llvm::Function* F, DefectManager* DM,
                                           SlotTrackerPass* ST, Statifier statify)
        : ObjectLevelLogging("ps-interpreter"), F_(F), DM_(DM), ST_(ST), statify_(statify) {
    FN_ = FactoryNest(F_->getParent()->getDataLayout(), ST_->getSlotTracker(F_));
}

void PSInterpreter::interpret() {
    if (interpreted_.count(F_)) return;
    interpreted_.insert(F_);

    auto module = absint::ir::Module(F_->getParent(), ST_);

    auto globFinder = GlobalVariableFinder(FN_);
    auto searched = globFinder.transform(statify_(&F_->back().back()));
    auto filtered = ContractPredicatesFilterer(FN_).transform(searched);
    module.initGlobals(globFinder.getGlobals());

    auto interpreter = absint::ps::Interpreter(FN_, module.getDomainFactory());
    interpreter.transform(filtered);
    auto&& checker = ps::OutOfBoundsChecker(FN_, DM_, module.getDomainFactory(), &interpreter);
    checker.transform(filtered);
    checker.apply();
    // TODO: add defects to DM_
}

bool PSInterpreter::hasInfo(const DefectInfo& info) {
    interpret();
    return DM_->hasInfo(info);
}

std::unordered_set<llvm::Function*> PSInterpreter::interpreted_;

}   // namespace absint
}   // namespace borealis
