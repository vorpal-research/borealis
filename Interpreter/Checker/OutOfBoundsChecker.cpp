//
// Created by abdullin on 7/6/17.
//

#include "OutOfBoundsChecker.h"

namespace borealis {
namespace absint {

OutOfBoundsChecker::OutOfBoundsChecker(Module* module, DefectManager* DM)
        : ObjectLevelLogging("interpreter"),
          module_(module),
          DM_(DM) {
    ST_ = module_->getSlotTracker();
}

void OutOfBoundsChecker::visitGEPOperator(llvm::Instruction& loc, llvm::GEPOperator& GI) {
    auto di = DM_->getDefect(DefectType::BUF_01, &loc);
    errs() << "Checking: " << ST_->toString(&loc) << endl;
    errs() << "Defect: " << di << endl;
    if (not module_->get(loc.getParent()->getParent())->getBasicBlock(loc.getParent())->isVisited()) {
        errs() << "Instruction not visited" << endl;
        DM_->addNoAbsIntDefect(di);
    } else {
        auto ptr = module_->getDomainFor(GI.getPointerOperand(), loc.getParent());
        if (not ptr) return;
        errs() << "Pointer operand: " << ptr << endl;
        for (auto j = GI.idx_begin(); j != GI.idx_end(); ++j) {
            auto indx = module_->getDomainFor(llvm::cast<llvm::Value>(j), loc.getParent());
            errs() << "Shift: " << indx << endl;
        }
    }
    errs() << endl;
}

void OutOfBoundsChecker::visitInstruction(llvm::Instruction& I) {
    for (auto&& op : util::viewContainer(I.operands())
            .map(llvm::dyn_caster<llvm::GEPOperator>())
            .filter()) {
        visitGEPOperator(I, *op);
    }
}

void OutOfBoundsChecker::visitGetElementPtrInst(llvm::GetElementPtrInst& GI) {
    visitGEPOperator(GI, llvm::cast<llvm::GEPOperator>(GI));
}

void OutOfBoundsChecker::run() {
    visit(const_cast<llvm::Module*>(module_->getInstance()));
}

} // namespace absint
} // namespace borealis
