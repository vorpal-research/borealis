//
// Created by abdullin on 2/10/17.
//

#include <memory>

#include "Interpreter.h"
#include "Interpreter/InstructionVisitor.h"

#include "Util/collections.hpp"
#include "Util/streams.hpp"

namespace borealis {
namespace absint {

Interpreter::Interpreter(const llvm::Module* module) : environment_(module), module_(&environment_, module) {}

void Interpreter::run() {
    for (auto&& f : util::viewContainer(*module_.getInstance())) {
        auto&& function = module_.getFunction(f);

        for (auto&& bb : util::viewContainer(f)) {
            auto&& basicBlock = function->getBasicBlock(&bb);
            basicBlock->getOutputState()->merge(basicBlock->getInputState());
            InstructionVisitor(&environment_, basicBlock->getOutputState()).visit(const_cast<llvm::BasicBlock&>(bb));
            for (auto&& pred : function->getSuccessorsFor(&bb)) {
                pred->getInputState()->merge(basicBlock->getOutputState());
            }

        }
        errs() << function->toString() << endl;
    }
}

}   /* namespace absint */
}   /* namespace borealis */
