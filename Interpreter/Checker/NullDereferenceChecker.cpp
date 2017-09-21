//
// Created by abdullin on 8/28/17.
//

#include "NullDereferenceChecker.h"

#include "Util/macros.h"

namespace borealis {
namespace absint {

static config::BoolConfigEntry enableLog("absint", "log-checker");

NullDereferenceChecker::NullDereferenceChecker(Module* module, DefectManager* DM)
        : ObjectLevelLogging("interpreter"),
          module_(module),
          DM_(DM) {
    ST_ = module_->getSlotTracker();
}

void NullDereferenceChecker::run() {
    visit(const_cast<llvm::Module*>(module_->getInstance()));

    util::viewContainer(defects_)
            .filter([&](auto&& it) -> bool { return not it.second; })
            .foreach([&](auto&& it) -> void { DM_->addNoAbsIntDefect(it.first); });
}

void NullDereferenceChecker::visitLoadInst(llvm::LoadInst& I) { checkPointer(I, *I.getPointerOperand()); }
void NullDereferenceChecker::visitStoreInst(llvm::StoreInst& I) { checkPointer(I, *I.getPointerOperand()); }
void NullDereferenceChecker::visitGetElementPtrInst(llvm::GetElementPtrInst& I) { checkPointer(I, *I.getPointerOperand()); }

void NullDereferenceChecker::checkPointer(llvm::Instruction& loc, llvm::Value& ptr) {
    auto&& info = infos();

    auto di = DM_->getDefect(DefectType::INI_03, &loc);
    if (enableLog.get(true)) {
        info << "Checking: " << ST_->toString(&loc) << endl;
        info << "Pointer operand: " << ST_->toString(&ptr) << endl;
        info << "Defect: " << di << endl;
    }

    if (not module_->checkVisited(&loc) || not module_->checkVisited(&ptr)) {
        if (enableLog.get(true)) info << "Instruction not visited" << endl;
        defects_[di] |= false;

    } else {
        auto&& ptr_domain = module_->getDomainFor(&ptr, loc.getParent());
        auto bug = not ptr_domain->isValue() || ptr_domain->isNullptr();
        if (enableLog.get(true)) {
            info << "Pointer domain: " << ptr_domain << endl;
            info << "Result: " << bug << endl;
        }
        defects_[di] |= bug;
    }
}

void NullDereferenceChecker::visitCallInst(llvm::CallInst& CI) {
    auto di = DM_->getDefect(DefectType::INI_03, &CI);
    if (CI.getCalledFunction() && CI.getCalledFunction()->isDeclaration()) {
        defects_[di] = true;
    }
}

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"