//
// Created by abdullin on 2/9/17.
//

#include "BasicBlock.h"

namespace borealis {
namespace absint {


BasicBlock::BasicBlock(const Environment* environment, const llvm::BasicBlock* bb) : environment_(environment),
                                                                                   instance_(bb) {
    inputState_ = std::make_shared<State>(State(environment_));
    outputState_ = std::make_shared<State>(State(environment_));
}

const llvm::BasicBlock* BasicBlock::getInstance() const {
    return instance_;
}

State::Ptr BasicBlock::getInputState() const {
    return inputState_;
}

State::Ptr BasicBlock::getOutputState() const {
    return outputState_;
}

std::string BasicBlock::getName() const {
    return (instance_->hasName()) ? instance_->getName().str() : environment_->getSlotTracker().getLocalName(instance_);
}

std::string BasicBlock::toString() const {
    std::ostringstream ss;

    ss << "*** basicBlock ";
    if (instance_->hasName())
        ss << instance_->getName().str();
    else
        ss << "<label>:" << environment_->getSlotTracker().getLocalSlot(instance_);

    ss << std::endl;
    ss << outputState_->toString() << std::endl;
    return ss.str();
}

}   /* namespace absint */
}   /* namespace borealis */