//
// Created by abdullin on 11/15/17.
//

#include "ContractChecker.h"
#include "Interpreter/PredicateState/State.h"
#include "State/Transformer/ContractChecker.h"

#include "Util/macros.h"

namespace borealis {
namespace absint {
namespace ir {

static config::BoolConfigEntry enableLogging("absint", "checker-logging");

ContractChecker::ContractChecker(Module* module, DefectManager* DM, FunctionManager* FM)
        : ObjectLevelLogging("ir-interpreter"),
          module_(module),
          DM_(DM),
          FM_(FM) {
    ST_ = module_->getSlotTracker();
}

void ContractChecker::run() {
    visit(const_cast<llvm::Module*>(module_->getInstance()));

    util::viewContainer(defects_)
            .filter(LAM(a, not a.second))
            .foreach([&](auto&& it) -> void { DM_->addNoAbsIntDefect(it.first); });
}

void ContractChecker::visitCallInst(llvm::CallInst& CI) {
    if (not CI.getCalledFunction()) return;
    if (not module_->checkVisited(&CI)) return;

    auto FN = FactoryNest();
    auto F = module_->get(CI.getCalledFunction());
    auto di = DM_->getDefect(DefectType::REQ_01, &CI);
    auto&& req = FM_->getReq(F->getInstance());
    if (req->isEmpty()) return;

    if (enableLogging.get(false)) {
        errs() << "Checking req of function " << F->getName() << endl;
        errs() << "Defect: " << di << endl;
        errs() << "Req: " << req << endl;
    }

    ps::State::Ptr state = std::make_shared<ps::State>();
    for (auto&& it : F->getGlobals()) {
        state->addVariable(FN.Term->getValueTerm(it), F->getDomainFor(it, CI.getParent()));
    }

    auto checker = ps::impl_::Checker(FN, module_->getDomainFactory(), state, F->getArguments());

    checker.transform(req);
    auto satisfied = checker.satisfied();
    defects_[di] |= (not satisfied);

    if (enableLogging.get(false)) {
        errs() << "Req satisfied: " << satisfied << endl;
        errs() << "Defect: " << (not satisfied) << endl;
        errs() << endl;
    }
}

void ContractChecker::visitReturnInst(llvm::ReturnInst& RI) {
    if (not module_->checkVisited(&RI)) return;

    auto FN = FactoryNest();
    auto F = module_->get(RI.getParent()->getParent());
    auto di = DM_->getDefect(DefectType::ENS_01, &RI);
    auto&& ens = FM_->getReq(F->getInstance());
    if (ens->isEmpty()) return;

    if (enableLogging.get(false)) {
        errs() << "Checking ens of function " << F->getName() << endl;
        errs() << "Defect: " << di << endl;
        errs() << "Ens: " << ens << endl;
    }

    ps::State::Ptr state = std::make_shared<ps::State>();
    for (auto&& it : F->getGlobals()) {
        state->addVariable(FN.Term->getValueTerm(it), F->getDomainFor(it, RI.getParent()));
    }

    auto checker = ps::impl_::Checker(FN, module_->getDomainFactory(), state, F->getArguments());

    checker.transform(ens);
    auto satisfied = checker.satisfied();
    defects_[di] |= (not satisfied);

    if (enableLogging.get(false)) {
        errs() << "Ens satisfied: " << satisfied << endl;
        errs() << "Defect: " << (not satisfied) << endl;
        errs() << endl;
    }
}

} // namespace ir
} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"