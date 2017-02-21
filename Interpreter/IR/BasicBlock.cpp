//
// Created by abdullin on 2/9/17.
//

#include "BasicBlock.h"

namespace borealis {
namespace absint {


BasicBlock::BasicBlock(const llvm::BasicBlock* bb, SlotTracker* tracker) : instance_(bb),
                                                                           tracker_(tracker),
                                                                           atFixpoint_(false) {
    inputState_ = State::Ptr{ new State() };
    outputState_ = State::Ptr{ new State() };
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
    return (instance_->hasName()) ? instance_->getName().str() : tracker_->getLocalName(instance_);
}

std::string BasicBlock::toString() const {
    std::ostringstream ss;

    if (instance_->hasName())
        ss << instance_->getName().str() << ":";
    else
        ss << "<label>:" << tracker_->getLocalSlot(instance_);

    ss << std::endl;
    ss << outputState_->toString(*tracker_);
    return ss.str();
}

bool BasicBlock::empty() const {
    return inputState_->empty() && outputState_->empty();
}

bool BasicBlock::atFixpoint() const {
    if (empty()) return false;
    if (atFixpoint_) return true;
    atFixpoint_ = inputState_->equals(outputState_.get());
    return atFixpoint_;
}

std::ostream& operator<<(std::ostream& s, const BasicBlock& b) {
    s << b.toString();
    return s;
}

borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const BasicBlock& b) {
    s << b.toString();
    return s;
}

}   /* namespace absint */
}   /* namespace borealis */