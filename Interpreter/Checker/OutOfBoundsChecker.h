//
// Created by abdullin on 7/6/17.
//

#ifndef BOREALIS_OUTOFBOUNDSCHECKER_H
#define BOREALIS_OUTOFBOUNDSCHECKER_H

#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Operator.h>

#include "Interpreter/IR/Module.h"
#include "Logging/logger.hpp"
#include "Passes/Defect/DefectManager.h"
#include "Passes/Tracker/SlotTrackerPass.h"

namespace borealis {
namespace absint {

class OutOfBoundsChecker : public llvm::InstVisitor<OutOfBoundsChecker>,
                           public logging::ObjectLevelLogging<OutOfBoundsChecker> {

public:

    OutOfBoundsChecker(Module* module, DefectManager* DM);

    void run();

    void visitInstruction(llvm::Instruction& I);
    void visitGetElementPtrInst(llvm::GetElementPtrInst& GI);

    void visitGEPOperator(llvm::Instruction& loc, llvm::GEPOperator& GI);

private:

    Module* module_;
    DefectManager* DM_;
    SlotTrackerPass* ST_;
    std::unordered_set<llvm::Value*> visited_;
    std::unordered_map<DefectInfo, bool> defects_;

};

} // namespace absint
} // namespace borealis

#endif //BOREALIS_OUTOFBOUNDSCHECKER_H
