//
// Created by abdullin on 2/9/17.
//

#include "BasicBlock.h"

#include "Util/collections.hpp"

namespace borealis {
namespace absint {


BasicBlock::BasicBlock(const llvm::BasicBlock* bb, SlotTracker* tracker) : instance_(bb),
                                                                           tracker_(tracker),
                                                                           atFixpoint_(false),
                                                                           visited_(false) {
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

    for (auto&& it : util::viewContainer(*instance_)) {
        auto value = llvm::cast<llvm::Value>(&it);
        auto domain = outputState_->find(value);
        if (not domain) continue;
        ss << std::endl << "  ";
        ss << tracker_->getLocalName(value) << " = ";
        ss << domain->toString("  ");
    }
    ss << std::endl;
    return ss.str();
}

std::string BasicBlock::toFullString() const {
    std::ostringstream ss;

    if (instance_->hasName())
        ss << instance_->getName().str() << ":";
    else
        ss << "<label>:" << tracker_->getLocalSlot(instance_);

    ss << outputState_->toString(*tracker_);
    ss << std::endl;
    return ss.str();
}

bool BasicBlock::empty() const {
    return inputState_->empty() && outputState_->empty();
}

bool BasicBlock::atFixpoint() {
    if (empty()) return false;
    if (atFixpoint_) return true;
    atFixpoint_ = inputState_->equals(outputState_.get());
    return atFixpoint_;
}

bool BasicBlock::isVisited() const {
    return visited_;
}

void BasicBlock::setVisited() {
    visited_ = true;
}

void BasicBlock::addPredecessor(BasicBlock* pred) {
    predecessors_.push_back(pred);
}

void BasicBlock::addSuccessor(BasicBlock* succ) {
    successors_.push_back(succ);
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