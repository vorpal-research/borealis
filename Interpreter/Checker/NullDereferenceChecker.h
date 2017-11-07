//
// Created by abdullin on 8/28/17.
//

#ifndef BOREALIS_NULLDEREFERENCECHECKER_H
#define BOREALIS_NULLDEREFERENCECHECKER_H

#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Operator.h>

#include "Interpreter/IR/Module.h"
#include "Logging/logger.hpp"
#include "Passes/Defect/DefectManager.h"
#include "Passes/Tracker/SlotTrackerPass.h"

namespace borealis {
namespace absint {
namespace ir {

class NullDereferenceChecker : public llvm::InstVisitor<NullDereferenceChecker>,
                               public logging::ObjectLevelLogging<NullDereferenceChecker> {

public:

    NullDereferenceChecker(Module* module, DefectManager* DM);

    void run();

    void checkPointer(llvm::Instruction& loc, llvm::Value& ptr);

    void visitLoadInst(llvm::LoadInst& I);
    void visitStoreInst(llvm::StoreInst& I);
    void visitGetElementPtrInst(llvm::GetElementPtrInst& I);
    void visitCallInst(llvm::CallInst& CI);

private:

    Module* module_;
    DefectManager* DM_;
    SlotTrackerPass* ST_;
    std::unordered_map<DefectInfo, bool> defects_;

};

} // namespace ir
} // namespace absint
} // namespace borealis

#endif //BOREALIS_NULLDEREFERENCECHECKER_H
