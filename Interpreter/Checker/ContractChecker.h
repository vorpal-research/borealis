//
// Created by abdullin on 11/15/17.
//

#ifndef BOREALIS_IR_CONTRACTCHECKER_H
#define BOREALIS_IR_CONTRACTCHECKER_H

#include <llvm/IR/InstVisitor.h>

#include "Interpreter/IR/Module.h"
#include "Logging/logger.hpp"
#include "Passes/Defect/DefectManager.h"
#include "Passes/Manager/FunctionManager.h"
#include "Passes/Tracker/SlotTrackerPass.h"

namespace borealis {
namespace absint {
namespace ir {

class ContractChecker : public llvm::InstVisitor<ContractChecker>,
                        public logging::ObjectLevelLogging<ContractChecker> {
public:

    ContractChecker(Module* module, DefectManager* DM, FunctionManager* FM);

    void run();

    void visitCallInst(llvm::CallInst& CI);
    void visitReturnInst(llvm::ReturnInst& RI);

private:
    Module* module_;
    DefectManager* DM_;
    FunctionManager* FM_;
    SlotTrackerPass* ST_;
    std::unordered_map<DefectInfo, bool> defects_;
};

} // namespace ir
} // namespace absint
} // namespace borealis


#endif //BOREALIS_IR_CONTRACTCHECKER_H
