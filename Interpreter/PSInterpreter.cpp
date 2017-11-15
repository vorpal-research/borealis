//
// Created by abdullin on 10/27/17.
//

#include "Interpreter/IR/Module.h"
#include "State/Transformer/ContractChecker.h"
#include "State/Transformer/GlobalVariableFinder.h"
#include "State/Transformer/PropertyPredicateFilterer.h"
#include "State/Transformer/Interpreter.h"
#include "State/Transformer/NullDereferenceChecker.h"
#include "State/Transformer/OutOfBoundsChecker.h"
#include "PSInterpreter.h"

namespace borealis {
namespace absint {

PSInterpreter::PSInterpreter(llvm::Function* F, DefectManager* DM,
                             SlotTrackerPass* ST, FunctionManager* FM, Statifier statify)
        : ObjectLevelLogging("ps-interpreter"), F_(F), DM_(DM), ST_(ST), FM_(FM), statify_(statify) {
    FN_ = FactoryNest(F_->getParent()->getDataLayout(), ST_->getSlotTracker(F_));
}

void PSInterpreter::interpret() {
    if (interpreted_.count(F_)) return;
    interpreted_.insert(F_);

    auto module = absint::ir::Module(F_->getParent(), ST_);

    auto globFinder = GlobalVariableFinder(FN_);
    auto searched = globFinder.transform(statify_(&F_->back().back()));
    auto filtered = PropertyPredicateFilterer(FN_).transform(searched);
    module.initGlobals(globFinder.getGlobals());

    auto interpreter = absint::ps::Interpreter(FN_, module.getDomainFactory());
    interpreter.transform(filtered);
    auto&& oob_checker = ps::OutOfBoundsChecker(FN_, DM_, module.getDomainFactory(), &interpreter);
    oob_checker.transform(filtered);
    oob_checker.apply();

    auto&& nd_checker = ps::NullDereferenceChecker(FN_, DM_, &interpreter);
    nd_checker.transform(filtered);
    nd_checker.apply();

    auto&& contract_checker = ps::ContractChecker(FN_, F_, DM_, FM_, module.getDomainFactory(), &interpreter);
    contract_checker.transform(filtered);
    contract_checker.apply();
}

bool PSInterpreter::hasInfo(const DefectInfo& info) {
    interpret();
    return DM_->hasInfo(info);
}

std::unordered_set<llvm::Function*> PSInterpreter::interpreted_;

}   // namespace absint
}   // namespace borealis
