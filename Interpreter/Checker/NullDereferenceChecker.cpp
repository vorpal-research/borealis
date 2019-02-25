//
// Created by abdullin on 8/28/17.
//

#include "Interpreter/Domain/AbstractFactory.hpp"
#include "Interpreter/Domain/Memory/PointerDomain.hpp"
#include "NullDereferenceChecker.h"

#include "Util/macros.h"

namespace borealis {
namespace absint {
namespace ir {

static config::BoolConfigEntry enableLogging("absint", "checker-logging");

NullDereferenceChecker::NullDereferenceChecker(Module* module, DefectManager* DM)
        : ObjectLevelLogging("ir-interpreter"),
          module_(module),
          DM_(DM) {
    ST_ = module_->slotTracker();
}

void NullDereferenceChecker::run() {
    visit(const_cast<llvm::Module*>(module_->instance()));

    util::viewContainer(defects_)
            .filter(LAM(a, not a.second))
            .foreach([&](auto&& it) -> void { DM_->addNoAbsIntDefect(it.first); });
}

void NullDereferenceChecker::visitLoadInst(llvm::LoadInst& I) { checkPointer(I, *I.getPointerOperand()); }
void NullDereferenceChecker::visitStoreInst(llvm::StoreInst& I) { checkPointer(I, *I.getPointerOperand()); }
void NullDereferenceChecker::visitGetElementPtrInst(llvm::GetElementPtrInst& I) { checkPointer(I, *I.getPointerOperand()); }

void NullDereferenceChecker::checkPointer(llvm::Instruction& loc, llvm::Value& ptr) {
    auto&& info = infos();

    auto di = DM_->getDefect(DefectType::INI_03, &loc);
    if (enableLogging.get(true)) {
        info << "Checking: " << ST_->toString(&loc) << endl;
        info << "Pointer operand: " << ST_->toString(&ptr) << endl;
        info << "Defect: " << di << endl;
    }

    if (not module_->checkVisited(&loc) || not module_->checkVisited(&ptr)) {
        if (enableLogging.get(true)) info << "Instruction not visited" << endl;
        defects_[di] |= false;

    } else {
        auto&& ptrDomain = module_->getDomainFor(&ptr, loc.getParent());
        auto&& ptr = llvm::dyn_cast<AbstractFactory::PointerT>(ptrDomain.get());
        auto bug = ptr->isTop() || ptr->isTop() || ptr->pointsToNull();
        if (enableLogging.get(true)) {
            info << "Pointer domain: " << ptrDomain << endl;
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

} // namespace ir
} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"