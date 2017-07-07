//
// Created by abdullin on 2/9/17.
//

#include "BasicBlock.h"

#include "Util/collections.hpp"

namespace borealis {
namespace absint {

BasicBlock::BasicBlock(const llvm::BasicBlock* bb, SlotTracker* tracker) : instance_(bb),
                                                                           tracker_(tracker),
                                                                           visited_(false) {
    inputState_ = State::Ptr{ new State(tracker_) };
    outputState_ = State::Ptr{ new State(tracker_) };
}

const llvm::BasicBlock* BasicBlock::getInstance() const {
    return instance_;
}

SlotTracker& BasicBlock::getSlotTracker() const {
    return *tracker_;
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
    ss << *this;
    return ss.str();
}

std::string BasicBlock::toFullString() const {
    std::ostringstream ss;
    ss << getName() << " output:";
    ss << outputState_->toString();
    ss << std::endl;
    return ss.str();
}

std::string BasicBlock::inputToString() const {
    std::ostringstream ss;
    ss << getName() << " input:";
    ss << inputState_->toString();
    ss << std::endl;
    return ss.str();
}


bool BasicBlock::empty() const {
    return inputState_->empty() && outputState_->empty();
}

bool BasicBlock::atFixpoint() {
    if (empty()) return false;
    return inputState_->equals(outputState_.get());
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

Domain::Ptr BasicBlock::getDomainFor(const llvm::Value* value) {
    return outputState_->find(value);
}

std::ostream& operator<<(std::ostream& s, const BasicBlock& b) {
    s << b.getName() << ":";
    for (auto&& it : util::viewContainer(*b.getInstance())) {
        auto value = llvm::cast<llvm::Value>(&it);
        auto domain = b.getOutputState()->find(value);
        if (not domain) continue;
        s << std::endl << "  "
          << b.getSlotTracker().getLocalName(value) << " = "
          << domain->toPrettyString("  ");
        s.flush();
    }
    s << std::endl;
    return s;
}

borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const BasicBlock& b) {
    s << b.getName() << ":";
    for (auto&& it : util::viewContainer(*b.getInstance())) {
        auto value = llvm::cast<llvm::Value>(&it);
        auto domain = b.getOutputState()->find(value);
        if (not domain) continue;

        s << endl << "  "
          << b.getSlotTracker().getLocalName(value) << " = "
          << domain->toPrettyString("  ");
        s.flush();
    }
    s << endl;
    return s;
}

std::ostream& operator<<(std::ostream& s, const BasicBlock* b) {
    s << *b;
    return s;
}

borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const BasicBlock* b) {
    s << *b;
    return s;
}

}   /* namespace absint */
}   /* namespace borealis */
