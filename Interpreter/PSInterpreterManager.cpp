//
// Created by abdullin on 10/27/17.
//

#include "Interpreter/IR/Module.h"
#include "State/Transformer/GlobalVariableFinder.h"
#include "State/Transformer/ContractPredicatesFilterer.h"
#include "State/Transformer/PSInterpreter.h"
#include "PSInterpreterManager.h"

namespace borealis {
namespace absint {

PSInterpreterManager::PSInterpreterManager(llvm::Function* F, DefectManager* DM,
                                           SlotTrackerPass* ST, Statifier statify)
        : ObjectLevelLogging("ps-interpreter"), F_(F), DM_(DM), ST_(ST), statify_(statify) {
    FN_ = FactoryNest(F_->getParent()->getDataLayout(), ST_->getSlotTracker(F_));
}

void PSInterpreterManager::interpret() {
    if (interpreted_.count(F_)) return;
    interpreted_.insert(F_);

    auto module = absint::Module(F_->getParent(), ST_);

    auto globFinder = GlobalVariableFinder(FN_);
    auto searched = globFinder.transform(statify_(&F_->back().back()));
    auto filtered = ContractPredicatesFilterer(FN_).transform(searched);
    module.initGlobals(globFinder.getGlobals());

    auto interpreter = absint::PSInterpreter(FN_, module.getDomainFactory());
    interpreter.transform(filtered);
    // TODO: add defects to DM_
}

bool PSInterpreterManager::hasInfo(const DefectInfo& info) {
    interpret();
    return DM_->hasInfo(info);
}

std::unordered_set<llvm::Function*> PSInterpreterManager::interpreted_;

}   // namespace absint
}   // namespace borealis
